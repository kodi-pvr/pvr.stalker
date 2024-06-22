/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#define HTTPSOCKET_MINIMUM_TIMEOUT 5

class HTTPSocket
{
public:
  typedef enum
  {
    SCOPE_REMOTE,
    SCOPE_LOCAL
  } Scope;

  typedef enum
  {
    METHOD_GET
  } Method;

  struct URLOption
  {
    std::string name;
    std::string value;
  };

  struct Request
  {
    Scope scope = SCOPE_REMOTE;
    Method method = METHOD_GET;
    std::string url;
    std::vector<URLOption> options;

    void AddURLOption(const std::string& name, const std::string& value)
    {
      URLOption option = {name, value};
      options.push_back(option);
    }
  };

  struct Response
  {
    bool useCache = false;
    std::string url;
    unsigned int expiry = 0;
    std::string body;
    bool writeToBody = true;
  };

  HTTPSocket(unsigned int timeout = HTTPSOCKET_MINIMUM_TIMEOUT);
  virtual ~HTTPSocket() = default;

  virtual bool Execute(Request& request, Response& response);

protected:
  virtual void SetDefaults(Request& request);

  virtual void BuildRequestURL(Request& request);

  virtual bool Get(Request& request, Response& response, bool reqUseCache);

  virtual bool ResponseIsFresh(Response& response);

  unsigned int m_timeout;
  std::vector<URLOption> m_defaultOptions;
};
