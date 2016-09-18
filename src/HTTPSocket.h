#pragma once

/*
 *      Copyright (C) 2015, 2016  Jamal Edey
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

#define HTTPSOCKET_MINIMUM_TIMEOUT 5

class HTTPSocket {
public:
    typedef enum {
        SCOPE_REMOTE,
        SCOPE_LOCAL
    } Scope;

    typedef enum {
        METHOD_GET
    } Method;

    struct URLOption {
        std::string name;
        std::string value;
    };

    struct Request {
        Scope scope = SCOPE_REMOTE;
        Method method = METHOD_GET;
        std::string url;
        std::vector<URLOption> options;

        void AddURLOption(const std::string &name, const std::string &value) {
            URLOption option = {name, value};
            options.push_back(option);
        }
    };

    struct Response {
        bool useCache = false;
        std::string url;
        unsigned int expiry = 0;
        std::string body;
        bool writeToBody = true;
    };

    HTTPSocket(unsigned int timeout = HTTPSOCKET_MINIMUM_TIMEOUT);

    virtual ~HTTPSocket();

    virtual bool Execute(Request &request, Response &response);

protected:
    virtual void SetDefaults(Request &request);

    virtual void BuildRequestURL(Request &request);

    virtual bool Get(Request &request, Response &response, bool reqUseCache);

    virtual bool ResponseIsFresh(Response &response);

    unsigned int m_timeout;
    std::vector<URLOption> m_defaultOptions;
};
