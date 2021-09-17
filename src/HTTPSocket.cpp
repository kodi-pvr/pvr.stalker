/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "HTTPSocket.h"

#include "Utils.h"

#include <kodi/Filesystem.h>
#include <kodi/tools/StringUtils.h>

#define TEMP_BUFFER_SIZE 1024

HTTPSocket::HTTPSocket(unsigned int timeout) : m_timeout(timeout)
{
  URLOption option;

  option = {"User-Agent", "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like "
                          "Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3"};
  m_defaultOptions.push_back(option);

  // <= zero disables timeout
  if (m_timeout > 0)
  {
    option = {"Connection-Timeout", std::to_string(m_timeout)};
    m_defaultOptions.push_back(option);
  }
}

void HTTPSocket::SetDefaults(Request& request)
{
  bool found;

  for (std::vector<URLOption>::iterator option = m_defaultOptions.begin();
       option != m_defaultOptions.end(); ++option)
  {
    found = false;

    for (std::vector<URLOption>::iterator it = request.options.begin(); it != request.options.end();
         ++it)
    {
      if ((found = !kodi::tools::StringUtils::CompareNoCase(it->name, option->name)))
        break;
    }

    if (!found)
      request.AddURLOption(option->name, option->value);
  }
}

void HTTPSocket::BuildRequestURL(Request& request)
{
  char buffer[TEMP_BUFFER_SIZE];

  std::string requestUrl(request.url);

  if (request.scope == SCOPE_LOCAL)
    return;

  SetDefaults(request);

  if (request.options.empty())
    return;

  requestUrl += "|";

  for (std::vector<URLOption>::iterator it = request.options.begin(); it != request.options.end();
       ++it)
  {
    sprintf(buffer, "%s=%s", it->name.c_str(), Utils::UrlEncode(it->value).c_str());
    requestUrl += buffer;

    if (it + 1 != request.options.end())
      requestUrl += "&";
  }

  request.url = requestUrl;
}

bool HTTPSocket::Get(Request& request, Response& response, bool reqUseCache)
{
  std::string reqUrl;
  kodi::vfs::CFile reqHdl;
  kodi::vfs::CFile resHdl;
  char buffer[TEMP_BUFFER_SIZE];
  ssize_t res;

  if (!reqUseCache)
  {
    BuildRequestURL(request);
    reqUrl = request.url;
  }
  else
  {
    reqUrl = response.url;
  }

  if (!reqHdl.OpenFile(reqUrl, 0))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: failed to open reqUrl=%s", __func__, reqUrl.c_str());
    return false;
  }

  if (!reqUseCache && response.useCache)
  {
    if (!resHdl.OpenFileForWrite(response.url, true))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: failed to open url=%s", __func__, response.url.c_str());
      return false;
    }
  }

  memset(buffer, 0, sizeof(buffer));
  while ((res = reqHdl.Read(buffer, sizeof(buffer) - 1)) > 0)
  {
    if (resHdl.IsOpen() && resHdl.Write(buffer, (size_t)res) == -1)
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: error when writing to url=%s", __func__,
                response.url.c_str());
      break;
    }
    if (response.writeToBody)
      response.body += buffer;
    memset(buffer, 0, sizeof(buffer));
  }

  return true;
}

bool HTTPSocket::ResponseIsFresh(Response& response)
{
  bool result(false);

  if (kodi::vfs::FileExists(response.url, false))
  {
    kodi::vfs::FileStatus fileStat;
    kodi::vfs::StatFile(response.url, fileStat);

    time_t now;
    time(&now);

    kodi::Log(ADDON_LOG_DEBUG, "%s: now=%d | st_mtime=%d", __func__, now,
              fileStat.GetModificationTime());

    result = (fileStat.GetModificationTime() + response.expiry) > now;
  }

  return result;
}

bool HTTPSocket::Execute(Request& request, Response& response)
{
  bool reqUseCache(false);
  bool result(false);

  if (response.useCache)
    reqUseCache = ResponseIsFresh(response);

  switch (request.method)
  {
    case METHOD_GET:
      result = Get(request, response, reqUseCache);
      break;
  }

  if (!result)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: request failed", __func__);
    return false;
  }

  if (response.writeToBody)
  {
    kodi::Log(ADDON_LOG_DEBUG, "%s: %s", __func__,
              response.body.substr(0, 512).c_str()); // 512 is max
  }

  return true;
}
