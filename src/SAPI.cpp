/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SAPI.h"

#include "HTTPSocket.h"
#include "Utils.h"
#include "libstalkerclient/itv.h"
#include "libstalkerclient/stb.h"
#include "libstalkerclient/watchdog.h"

#include <kodi/General.h>
#include <memory>
#include <sstream>

#define SC_SAPI_AUTHORIZATION_FAILED "Authorization failed."

using namespace SC;

void SAPI::SetEndpoint(const std::string& endpoint)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::string server;
  size_t startPos;
  size_t pos;

  if ((startPos = endpoint.find("://")) == std::string::npos)
  {
    server = "http://";
    startPos = 4;
  }
  server += endpoint;
  startPos += 3;

  // xpcom.common.js > get_server_params()
  if ((pos = server.substr(startPos).find_last_of('/')) == std::string::npos)
  {
    server += '/';
    pos = server.length() - startPos;
  }
  pos += startPos;

  if (server.substr(pos - 2, 3).compare("/c/") == 0 &&
      server.substr(pos + 1).find(".php") == std::string::npos)
  {
    // strip tail from url path and set endpoint and referer
    m_basePath = server.substr(0, pos - 1);
    m_endpoint = m_basePath + "server/load.php";
    m_referer = server.substr(0, pos + 1);
  }
  else
  {
    m_basePath = server.substr(0, pos + 1);
    m_endpoint = server;
    m_referer = m_basePath;
  }

  kodi::Log(ADDON_LOG_DEBUG, "%s: m_basePath=%s", __func__, m_basePath.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "%s: m_endpoint=%s", __func__, m_endpoint.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "%s: m_referer=%s", __func__, m_referer.c_str());
}

bool SAPI::STBHandshake(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(STB_HANDSHAKE);
  if (!sc_stb_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_stb_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if (strlen(m_identity->token) > 0 && (param = sc_param_get(params, "token")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->token);
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::STBGetProfile(bool authSecondStep, Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(STB_GET_PROFILE);
  if (!sc_stb_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_stb_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if ((param = sc_param_get(params, "auth_second_step")))
    param->value.boolean = authSecondStep;

  if ((param = sc_param_get(params, "not_valid_token")))
    param->value.boolean = !m_identity->valid_token;

  if (strlen(m_identity->serial_number) > 0 && (param = sc_param_get(params, "sn")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->serial_number);
  }

  if ((param = sc_param_get(params, "device_id")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->device_id);
  }

  if ((param = sc_param_get(params, "device_id2")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->device_id2);
  }

  if ((param = sc_param_get(params, "signature")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->signature);
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::STBDoAuth(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(STB_DO_AUTH);
  if (!sc_stb_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_stb_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if ((param = sc_param_get(params, "login")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char*)m_identity->login);
  }

  if ((param = sc_param_get(params, "password")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char*)m_identity->password);
  }

  if ((param = sc_param_get(params, "device_id")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->device_id);
  }

  if ((param = sc_param_get(params, "device_id2")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(m_identity->device_id2);
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::ITVGetAllChannels(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  SError ret(SERROR_OK);

  params = sc_param_params_create(ITV_GET_ALL_CHANNELS);
  if (!sc_itv_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_itv_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::ITVGetOrderedList(int genre, int page, Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(ITV_GET_ORDERED_LIST);
  if (!sc_itv_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_itv_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if ((param = sc_param_get(params, "genre")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char*)std::to_string(genre).c_str());
  }

  if ((param = sc_param_get(params, "p")))
  {
    param->value.integer = page;
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::ITVCreateLink(std::string& cmd, Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(ITV_CREATE_LINK);
  if (!sc_itv_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_itv_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if ((param = sc_param_get(params, "cmd")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char*)cmd.c_str());
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::ITVGetGenres(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  SError ret(SERROR_OK);

  params = sc_param_params_create(ITV_GET_GENRES);
  if (!sc_itv_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_itv_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

bool SAPI::ITVGetEPGInfo(int period,
                         Json::Value& parsed,
                         const std::string& cacheFile,
                         unsigned int cacheExpiry)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(ITV_GET_EPG_INFO);
  if (!sc_itv_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_itv_defaults failed", __func__);
    sc_param_params_free(&params);
    return false;
  }

  if ((param = sc_param_get(params, "period")))
  {
    param->value.integer = period;
  }

  ret = StalkerCall(params, parsed, cacheFile, cacheExpiry);

  sc_param_params_free(&params);

  return ret == SERROR_OK;
}

SError SAPI::WatchdogGetEvents(int curPlayType, int eventActiveId, Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_param_params_t* params;
  sc_param_t* param;
  SError ret(SERROR_OK);

  params = sc_param_params_create(WATCHDOG_GET_EVENTS);
  if (!sc_watchdog_defaults(params))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: sc_watchdog_defaults failed", __func__);
    sc_param_params_free(&params);
    return SERROR_API;
  }

  if ((param = sc_param_get(params, "cur_play_type")))
    param->value.integer = curPlayType;

  if ((param = sc_param_get(params, "event_active_id")))
    param->value.integer = eventActiveId;

  ret = StalkerCall(params, parsed);

  sc_param_params_free(&params);

  return ret;
}

SError SAPI::StalkerCall(sc_param_params_t* params,
                         Json::Value& parsed,
                         const std::string& cacheFile,
                         unsigned int cacheExpiry)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  sc_request_t scRequest;
  sc_request_nameVal_t* scNameVal;
  std::ostringstream oss;
  HTTPSocket::Request request;
  HTTPSocket::Response response;
  HTTPSocket sock(m_timeout);
  std::string jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

  memset(&scRequest, 0, sizeof(scRequest));
  if (!sc_request_build(m_identity, params, &scRequest))
    kodi::Log(ADDON_LOG_ERROR, "sc_request_build failed");

  scNameVal = scRequest.headers;
  while (scNameVal)
  {
    request.AddURLOption(scNameVal->name, scNameVal->value);

    scNameVal = scNameVal->next;
  }

  request.AddURLOption("Referer", m_referer);
  request.AddURLOption("X-User-Agent", "Model: MAG250; Link: WiFi");

  sc_request_free_nameVals(&scRequest.headers);

  oss << m_endpoint << "?";
  scNameVal = scRequest.params;
  while (scNameVal)
  {
    oss << scNameVal->name << "=";
    oss << Utils::UrlEncode(std::string(scNameVal->value));

    if (scNameVal->next)
      oss << "&";

    scNameVal = scNameVal->next;
  }

  sc_request_free_nameVals(&scRequest.params);

  request.url = oss.str();
  response.useCache = cacheFile.length() > 0;
  response.url = cacheFile;
  response.expiry = cacheExpiry;

  if (!sock.Execute(request, response))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: api call failed", __func__);
    return SERROR_API;
  }
  if (!reader->parse(response.body.c_str(), response.body.c_str() + response.body.size(), &parsed,
                     &jsonReaderError))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: parsing failed", __func__);
    if (response.body.compare(SC_SAPI_AUTHORIZATION_FAILED) == 0)
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: authorization failed", __func__);
      return SERROR_AUTHORIZATION;
    }
    return SERROR_UNKNOWN;
  }

  return SERROR_OK;
}
