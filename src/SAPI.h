/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Error.h"
#include "libstalkerclient/identity.h"
#include "libstalkerclient/request.h"

#include <json/json.h>
#include <string>

namespace SC
{
class SAPI
{
public:
  SAPI() = default;
  virtual ~SAPI() = default;

  virtual void SetIdentity(sc_identity_t* identity) { m_identity = identity; }

  virtual void SetEndpoint(const std::string& endpoint);

  virtual std::string GetBasePath() { return m_basePath; }

  virtual void SetTimeout(unsigned int timeout) { m_timeout = timeout; }

  virtual bool STBHandshake(Json::Value& parsed);

  virtual bool STBGetProfile(bool authSecondStep, Json::Value& parsed);

  virtual bool STBDoAuth(Json::Value& parsed);

  virtual bool ITVGetAllChannels(Json::Value& parsed);

  virtual bool ITVGetOrderedList(int genre, int page, Json::Value& parsed);

  virtual bool ITVCreateLink(std::string& cmd, Json::Value& parsed);

  virtual bool ITVGetGenres(Json::Value& parsed);

  virtual bool ITVGetEPGInfo(int period,
                             Json::Value& parsed,
                             const std::string& cacheFile = "",
                             unsigned int cacheExpiry = 0);

  virtual SError WatchdogGetEvents(int curPlayType, int eventActiveId, Json::Value& parsed);

protected:
  virtual SError StalkerCall(sc_param_params_t* params,
                             Json::Value& parsed,
                             const std::string& cacheFile = "",
                             unsigned int cacheExpiry = 0);

private:
  sc_identity_t* m_identity = nullptr;
  std::string m_endpoint;
  std::string m_basePath;
  std::string m_referer;
  unsigned int m_timeout = 0;
};
} // namespace SC
