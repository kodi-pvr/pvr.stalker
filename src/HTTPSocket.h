#pragma once

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

#include <cstdint>
#include <string>
#include <vector>

#define MINIMUM_TIMEOUT 5

typedef enum
{
  REMOTE,
  LOCAL
} Scope;

typedef enum
{
  GET
} Method;

struct UrlOption
{
  std::string name;
  std::string value;
};

struct Request
{
  Scope                   scope   = REMOTE;
  Method                  method  = GET;
  std::string             url;
  std::vector<UrlOption>  options;
  std::string             body;
  
  void AddHeader(const std::string &name, const std::string &value)
  {
    UrlOption option = { name.c_str(), value.c_str() };
    options.push_back(option);
  }
};

struct Response
{
  std::string headers;
  std::string body;
};

class HTTPSocket
{
public:
  HTTPSocket(uint32_t iTimeout = MINIMUM_TIMEOUT);
  virtual ~HTTPSocket();

  virtual bool Execute(Request &request, Response &response);
protected:
  virtual void SetDefaults(Request &request);
  virtual void BuildRequestUrl(Request &request, std::string &strRequestUrl);
  virtual bool Get(std::string &strRequestUrl, std::string &strResponse);

  uint32_t                m_iTimeout;
  std::vector<UrlOption>  m_defaultOptions;
};

namespace PLATFORM
{
  class CTcpConnection;
}

class HTTPSocketRaw : public HTTPSocket
{
public:
  HTTPSocketRaw(uint32_t iTimeout = MINIMUM_TIMEOUT);
  ~HTTPSocketRaw();
  
  void SetURL(const std::string &url);
  bool Execute(Request &request, Response &response);
protected:
  void BuildRequestString(Request &request, std::string &strRequest);
  bool Open();
  void Close();
private:
  std::string               m_host;
  int                       m_port;
  PLATFORM::CTcpConnection  *m_socket;
};
