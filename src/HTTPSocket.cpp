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

#if TARGET_WINDOWS

#pragma warning(disable:4005) // Disable "warning C4005: '_WINSOCKAPI_' : macro redefinition"
#include <winsock2.h>
#pragma warning(default:4005)

#define close(s) closesocket(s)

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#endif

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

  sprintf(buffer, "%s %s HTTP/1.0\r\n", "GET", m_uri.c_str());
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

bool HTTPSocket::OpenSocket(int *sockfd)
{
  struct sockaddr_in servaddr;
  struct hostent *server;

  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    XBMC->Log(LOG_ERROR, "failed to create socket");
    return false;
  }

  if ((server = gethostbyname(m_host.c_str())) == NULL) {
    XBMC->Log(LOG_ERROR, "failed to resolve host");
    return false;
  }

  servaddr.sin_family = AF_INET;
  memcpy((char *)&servaddr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  servaddr.sin_port = htons(m_port);

  if (connect(*sockfd, (const sockaddr *)&servaddr, sizeof(sockaddr_in)) < 0) {
    XBMC->Log(LOG_ERROR, "failed to connect");
    return false;
  }

  return true;
}

bool HTTPSocket::CloseSocket(int *sockfd)
{
  if (close(*sockfd) < 0) {
    return false;
  }

  return true;
}

bool HTTPSocket::Execute(std::string *resp_headers, std::string *resp_body)
{
  std::string request;
  std::string response;
  int sockfd;
  char buffer[TEMP_BUFFER_SIZE];
  int len;
  size_t pos;

  if (!BuildRequest(&request)) {
    XBMC->Log(LOG_ERROR, "%s: failed to build request", __FUNCTION__);
    return false;
  }

  /*if (!OpenSocket(&sockfd)) {
    XBMC->Log(LOG_ERROR, "%s: failed to open socket", __FUNCTION__);
    return false;
  }*/

  uint64_t iNow = GetTimeMs();
  uint64_t iTarget = iNow + 5 * 1000;
  m_socket = new CTcpConnection(m_host.c_str(), m_port);
  //m_socket->Open();
  while (!m_socket->IsOpen() && iNow < iTarget)
  {
    if (!m_socket->Open(iTarget - iNow))
      CEvent::Sleep(100);
    iNow = GetTimeMs();
  }

  if (!m_socket->IsOpen())
  {
    XBMC->Log(LOG_ERROR, "%s - failed to connect to the backend (%s)", __FUNCTION__, m_socket->GetError().c_str());
    return false;
  }

  /*if ((len = send(sockfd, request.c_str(), strlen(request.c_str()), 0)) < 0) {
    XBMC->Log(LOG_ERROR, "%s: failed to write data", __FUNCTION__);
    return false;
  }*/

  if ((len = m_socket->Write((void*)request.c_str(), strlen(request.c_str()))) < 0) {
    XBMC->Log(LOG_ERROR, "%s: failed to write data", __FUNCTION__);
    return false;
  }

  /*memset(buffer, 0, TEMP_BUFFER_SIZE);
  while ((len = recv(sockfd, buffer, TEMP_BUFFER_SIZE - 1, 0)) > 0) {
    //buffer[len] = 0;
    response.append(buffer, len);
    memset(buffer, 0, TEMP_BUFFER_SIZE);
  }*/

  memset(buffer, 0, TEMP_BUFFER_SIZE);
  while ((len = m_socket->Read(buffer, TEMP_BUFFER_SIZE - 1)) > 0) {
    response.append(buffer, len);
    memset(buffer, 0, TEMP_BUFFER_SIZE);
  }

  /*if (!CloseSocket(&sockfd)) {
    XBMC->Log(LOG_ERROR, "%s: failed to close socket", __FUNCTION__);
    return false;
  }*/

  m_socket->Close();
  delete m_socket;
  m_socket = NULL;

  if ((pos = response.find("\r\n\r\n")) == std::string::npos) {
    XBMC->Log(LOG_ERROR, "%s: failed to split http response", __FUNCTION__);
    return false;
  }

  *resp_headers = response.substr(0, pos);
  *resp_body = response.substr(pos + 4);

  XBMC->Log(LOG_DEBUG, "%s", resp_headers->c_str());
  XBMC->Log(LOG_DEBUG, "%s", resp_body->substr(0, 512).c_str()); // 512 is max

  return true;
}
