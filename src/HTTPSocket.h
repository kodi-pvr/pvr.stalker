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

#include <string>
#include <vector>

namespace PLATFORM
{
  class CTcpConnection;
}

class HTTPSocket
{
public:
  HTTPSocket();
  virtual ~HTTPSocket();

  virtual void SetURL(const std::string &url);
  virtual void SetBody(const std::string &req_body);
  virtual void AddHeader(const std::string &name, const std::string &value);
  virtual bool Execute(std::string *resp_headers, std::string *resp_body);
protected:
  virtual bool BuildRequest(std::string *request);
  virtual bool Open();
  virtual void Close();
private:
  std::string               m_method;
  std::string               m_uri;
  std::string               m_host;
  int                       m_port;
  std::string               m_user_agent;
  std::vector<std::string>  m_headers;
  std::string               m_req_body;
  PLATFORM::CTcpConnection  *m_socket;
};
