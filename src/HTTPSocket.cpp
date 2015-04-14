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

#include <platform/sockets/tcp.h>

#include "client.h"

#define TEMP_BUFFER_SIZE 1024

using namespace ADDON;
using namespace PLATFORM;

HTTPSocket::HTTPSocket()
{
  m_method = "GET";
  m_port = 80;
  m_user_agent = "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3";
}

HTTPSocket::~HTTPSocket()
{
}

void HTTPSocket::SetURL(const std::string &url)
{
  std::string uri = "/";
  std::string host = url;
  int port = m_port;
  size_t pos;

  if (host.find("http://") == 0) {
    host.replace(0, 7, "");
  }
  if ((pos = host.find("/")) != std::string::npos) {
    uri = host.substr(pos);
    host.replace(pos, std::string::npos, "");
  }
  if ((pos = host.find(":")) != std::string::npos) {
    std::string sport = host.substr(pos + 1);
    long int lport = strtol(sport.c_str(), NULL, 10);
    port = lport != 0L ? (int)lport : port;
    host.replace(pos, std::string::npos, "");
  }

  m_uri = uri;
  m_host = host;
  m_port = port;
}

void HTTPSocket::AddHeader(const std::string &name, const std::string &value)
{
  char buffer[TEMP_BUFFER_SIZE];

  sprintf(buffer, "%s: %s", name.c_str(), value.c_str());
  m_headers.push_back(buffer);
}

void HTTPSocket::SetBody(const std::string &req_body)
{
  m_req_body = req_body;
}

bool HTTPSocket::BuildRequest(std::string *request)
{
  char buffer[TEMP_BUFFER_SIZE];

  sprintf(buffer, "%s %s HTTP/1.0\r\n", m_method.c_str(), m_uri.c_str());
  request->append(buffer);

  sprintf(buffer, "Host: %s:%d\r\n", m_host.c_str(), m_port);
  request->append(buffer);

  sprintf(buffer, "User-Agent: %s\r\n", m_user_agent.c_str());
  request->append(buffer);

  sprintf(buffer, "Accept: %s\r\n", "*/*");
  request->append(buffer);

  for (std::vector<std::string>::iterator it = m_headers.begin(); it != m_headers.end(); ++it) {
    request->append((*it) + "\r\n");
  }

  request->append("\r\n\r\n");

  request->append(m_req_body);

  return true;
}

bool HTTPSocket::Open()
{
  uint64_t iNow;
  uint64_t iTarget;

  iNow = GetTimeMs();
  iTarget = iNow + 5 * 1000;

  m_socket = new CTcpConnection(m_host.c_str(), m_port);

  while (!m_socket->IsOpen() && iNow < iTarget) {
    if (!m_socket->Open(iTarget - iNow))
      CEvent::Sleep(100);
    iNow = GetTimeMs();
  }

  if (!m_socket->IsOpen()) {
    XBMC->Log(LOG_ERROR, "%s: failed to connect", __FUNCTION__, m_socket->GetError().c_str());
    return false;
  }

  return true;
}

void HTTPSocket::Close()
{
  if (m_socket->IsOpen()) {
    m_socket->Close();
  }

  if (m_socket) {
    delete m_socket;
    m_socket = NULL;
  }
}

bool HTTPSocket::Execute(std::string *resp_headers, std::string *resp_body)
{
  std::string request;
  std::string response;
  char buffer[TEMP_BUFFER_SIZE];
  int len;
  size_t pos;

  if (!BuildRequest(&request)) {
    XBMC->Log(LOG_ERROR, "%s: failed to build request", __FUNCTION__);
    return false;
  }

  if (!Open()) {
    return false;
  }

  if ((len = m_socket->Write((void*)request.c_str(), strlen(request.c_str()))) < 0) {
    XBMC->Log(LOG_ERROR, "%s: failed to write request", __FUNCTION__);
    return false;
  }

  memset(buffer, 0, TEMP_BUFFER_SIZE);
  while ((len = m_socket->Read(buffer, TEMP_BUFFER_SIZE - 1)) > 0) {
    response.append(buffer, len);
    memset(buffer, 0, TEMP_BUFFER_SIZE);
  }

  Close();

  if ((pos = response.find("\r\n\r\n")) == std::string::npos) {
    XBMC->Log(LOG_ERROR, "%s: failed to parse response", __FUNCTION__);
    return false;
  }

  *resp_headers = response.substr(0, pos);
  *resp_body = response.substr(pos + 4);

  XBMC->Log(LOG_DEBUG, "%s", resp_headers->c_str());
  XBMC->Log(LOG_DEBUG, "%s", resp_body->substr(0, 512).c_str()); // 512 is max

  return true;
}
