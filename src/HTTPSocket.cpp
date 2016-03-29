/*
 *      Copyright (C) 2015  Jamal Edey
 *      http://www.kenshisoft.com/
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include "HTTPSocket.h"

#include "p8-platform/util/StringUtils.h"
#include "p8-platform/sockets/tcp.h"

#include "client.h"
#include "Utils.h"

#define TEMP_BUFFER_SIZE 1024

using namespace ADDON;

HTTPSocket::HTTPSocket(uint32_t iTimeout)
  : m_iTimeout(iTimeout)
{
  UrlOption option;

  option = { "User-Agent", "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3" };
  m_defaultOptions.push_back(option);

  // <= zero disables timeout
  if (m_iTimeout > 0) {
    option = { "Connection-Timeout", Utils::ToString(m_iTimeout) };
    m_defaultOptions.push_back(option);
  }
}

HTTPSocket::~HTTPSocket()
{
  m_defaultOptions.clear();
}

void HTTPSocket::SetDefaults(Request &request)
{
  bool found;
  
  for (std::vector<UrlOption>::iterator option = m_defaultOptions.begin(); option != m_defaultOptions.end(); ++option) {
    found = false;

    for (std::vector<UrlOption>::iterator it = request.options.begin(); it != request.options.end(); ++it) {
      std::string oname = option->name; StringUtils::ToLower(oname);
      std::string iname = it->name; StringUtils::ToLower(iname);
      
      if (found = (iname.compare(oname) == 0))
        break;
    }

    if (!found)
      request.AddHeader(option->name, option->value);
  }
}

void HTTPSocket::BuildRequestUrl(Request &request, std::string &strRequestUrl)
{
  char buffer[TEMP_BUFFER_SIZE];
  
  strRequestUrl += request.url;
  
  if (request.scope == LOCAL)
    return;
  
  SetDefaults(request);
  
  if (request.options.empty())
    return;
  
  strRequestUrl += "|";

  for (std::vector<UrlOption>::iterator it = request.options.begin(); it != request.options.end(); ++it) {
    sprintf(buffer, "%s=%s", it->name.c_str(), Utils::UrlEncode(it->value).c_str());
    strRequestUrl += buffer;

    if (it + 1 != request.options.end())
      strRequestUrl += "&";
  }
}

bool HTTPSocket::Get(std::string &strRequestUrl, std::string &strResponse)
{
  void *hdl;
  char buffer[1024];
  
  hdl = XBMC->OpenFile(strRequestUrl.c_str(), 0);
  if (hdl) {
    memset(buffer, 0, sizeof(buffer));
    while (XBMC->ReadFileString(hdl, buffer, sizeof(buffer) - 1)) {
      strResponse += buffer;
      memset(buffer, 0, sizeof(buffer));
    }
    
    XBMC->CloseFile(hdl);
  }
  
  return true;
}

bool HTTPSocket::Execute(Request &request, Response &response)
{
  std::string strRequestUrl;
  bool result;
  void *hdl;
  
  if (request.scope == REMOTE && request.method == GET
    && request.cache && XBMC->FileExists(request.cacheFile.c_str(), true))
  {
    struct __stat64 statCached;
    XBMC->StatFile(request.cacheFile.c_str(), &statCached);
    
    time_t now;
    time(&now);
    
    XBMC->Log(LOG_DEBUG, "%s: now=%d | st_mtime=%d",
      __FUNCTION__, now, statCached.st_mtime);
    
    request.cache = (statCached.st_mtime + request.cacheExpiry) < now;
    if (!request.cache) {
      // override the request and load locally from cache file instead
      request.scope = LOCAL;
      request.url = request.cacheFile;
      request.cache = false;
    }
  }
  
  BuildRequestUrl(request, strRequestUrl);
  
  switch (request.method) {
    case GET:
      result = Get(strRequestUrl, response.body);
      break;
    //case POST: //TODO
  }
  
  if (!result) {
    XBMC->Log(LOG_ERROR, "%s: request failed", __FUNCTION__);
    return false;
  }
  
  if (request.scope == REMOTE && request.cache && !request.cacheFile.empty()) {
    hdl = XBMC->OpenFileForWrite(request.cacheFile.c_str(), true);
    if (hdl) {
      ssize_t bytes = XBMC->WriteFile(hdl, response.body.c_str(), response.body.size());
      if (bytes == -1)
        XBMC->Log(LOG_ERROR, "%s: error when writing to file: %s=",
          __FUNCTION__, request.cacheFile.c_str());
      XBMC->CloseFile(hdl);
    } else {
      XBMC->Log(LOG_ERROR, "%s: failed to open file: %s=",
        __FUNCTION__, request.cacheFile.c_str());
    }
  }
  
  XBMC->Log(LOG_DEBUG, "%s", response.body.substr(0, 512).c_str()); // 512 is max
  
  return true;
}
