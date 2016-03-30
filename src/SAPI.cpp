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

#include "SAPI.h"

#include <algorithm>
#include <sstream>
 
#include "p8-platform/os.h"

#include "libstalkerclient/itv.h"
#include "libstalkerclient/param.h"
#include "libstalkerclient/stb.h"
#include "libstalkerclient/util.h"
#include "libstalkerclient/watchdog.h"
#include "client.h"
#include "Utils.h"

using namespace ADDON;

bool SAPI::Init()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::string strServer;
  size_t startPos;
  size_t pos;

  if ((startPos = g_strServer.find("://")) == std::string::npos) {
    strServer = "http://";
    startPos = 4;
  }
  strServer += g_strServer;
  startPos += 3;

  // xpcom.common.js > get_server_params()
  if ((pos = strServer.substr(startPos).find_last_of('/')) == std::string::npos) {
    strServer += '/';
    pos = strServer.length() - startPos;
  }
  pos += startPos;

  if (strServer.substr(pos - 2, 3).compare("/c/") == 0
    && strServer.substr(pos + 1).find(".php") == std::string::npos)
  {
    // strip tail from url path and set endpoint and referer
    g_strBasePath = strServer.substr(0, pos - 1);
    g_strEndpoint = g_strBasePath + "server/load.php";
    g_strReferer = strServer.substr(0, pos + 1);
  } else {
    g_strBasePath = strServer.substr(0, pos + 1);
    g_strEndpoint = strServer;
    g_strReferer = g_strBasePath;
  }

  XBMC->Log(LOG_DEBUG, "%s: g_strBasePath=%s", __FUNCTION__, g_strBasePath.c_str());
  XBMC->Log(LOG_DEBUG, "%s: g_strEndpoint=%s", __FUNCTION__, g_strEndpoint.c_str());
  XBMC->Log(LOG_DEBUG, "%s: g_strReferer=%s", __FUNCTION__, g_strReferer.c_str());

  return true;
}

SError SAPI::StalkerCall(sc_identity_t &identity, sc_param_request_t &params, Response &response, Json::Value &parsed,
  bool bCache, std::string strCacheFile, uint32_t cacheExpiry)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_request_t scRequest;
  sc_request_nameVal_t *scNameVal;
  std::ostringstream oss;
  Request request;
  HTTPSocket sock(g_iConnectionTimeout);
  Json::Reader reader;

  memset(&scRequest, 0, sizeof(scRequest));
  if (!sc_request_build(&identity, &params, &scRequest))
    XBMC->Log(LOG_ERROR, "sc_request_build failed");

  scNameVal = scRequest.headers;
  while (scNameVal) {
    request.AddURLOption(scNameVal->name, scNameVal->value);

    scNameVal = scNameVal->next;
  }

  request.AddURLOption("Referer", g_strReferer);
  request.AddURLOption("X-User-Agent", "Model: MAG250; Link: WiFi");

  sc_request_free_nameVals(scRequest.headers);

  oss << g_strEndpoint << "?";
  scNameVal = scRequest.params;
  while (scNameVal) {
    oss << scNameVal->name << "=";
    oss << Utils::UrlEncode(std::string(scNameVal->value));

    if (scNameVal->next)
      oss << "&";

    scNameVal = scNameVal->next;
  }

  sc_request_free_nameVals(scRequest.params);

  request.url = oss.str();
  response.useCache = bCache;
  response.url = strCacheFile;
  response.expiry = cacheExpiry;

  if (!sock.Execute(request, response)) {
    XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
    return SERROR_API;
  }

  if (!reader.parse(response.body, parsed)) {
    XBMC->Log(LOG_ERROR, "%s: parsing failed", __FUNCTION__);
    if (response.body.compare(AUTHORIZATION_FAILED) == 0) {
      XBMC->Log(LOG_ERROR, "%s: authorization failed", __FUNCTION__);
      return SERROR_AUTHORIZATION;
    }
    return SERROR_UNKNOWN;
  }

  return SERROR_OK;
}

bool SAPI::Handshake(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = STB_HANDSHAKE;

  if (!sc_stb_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_stb_defaults failed", __FUNCTION__);
    return false;
  }

  if (strlen(identity.token) > 0
    && (param = sc_param_get(&params, "token")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.token);
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::GetProfile(sc_identity_t &identity, bool bAuthSecondStep, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = STB_GET_PROFILE;

  if (!sc_stb_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_stb_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "auth_second_step")))
    param->value.boolean = bAuthSecondStep;

  if ((param = sc_param_get(&params, "not_valid_token")))
    param->value.boolean = !identity.valid_token;
  
  if (strlen(identity.serial_number) > 0
    && (param = sc_param_get(&params, "sn")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.serial_number);
  }
  
  if ((param = sc_param_get(&params, "device_id"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id);
  }
  
  if ((param = sc_param_get(&params, "device_id2"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id2);
  }
  
  if ((param = sc_param_get(&params, "signature"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.signature);
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::DoAuth(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = STB_DO_AUTH;

  if (!sc_stb_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_stb_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "login"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char *)identity.login);
  }

  if ((param = sc_param_get(&params, "password"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char *)identity.password);
  }
  
  if ((param = sc_param_get(&params, "device_id"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id);
  }
  
  if ((param = sc_param_get(&params, "device_id2"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id2);
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::GetAllChannels(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_ALL_CHANNELS;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::GetOrderedList(int iGenre, int iPage, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_ORDERED_LIST;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "genre"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char *)Utils::ToString(iGenre).c_str());
  }

  if ((param = sc_param_get(&params, "p"))) {
    param->value.integer = iPage;
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::CreateLink(std::string &cmd, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = ITV_CREATE_LINK;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "cmd"))) {
    free(param->value.string);
    param->value.string = sc_util_strcpy((char *)cmd.c_str());
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::GetGenres(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_GENRES;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret == SERROR_OK;
}

bool SAPI::GetEPGInfo(int iPeriod, sc_identity_t &identity, Json::Value &parsed,
  bool bCache, uint32_t cacheExpiry)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  std::string strCacheFile;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_EPG_INFO;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "period"))) {
    param->value.integer = iPeriod;
  }

  strCacheFile = Utils::GetFilePath("epg_provider.json");

  ret = StalkerCall(identity, params, response, parsed,
    bCache, strCacheFile, cacheExpiry);

  sc_param_free_params(params.param);

  if (ret != SERROR_OK && XBMC->FileExists(strCacheFile.c_str(), false)) {
#ifdef TARGET_WINDOWS
    DeleteFile(strCacheFile.c_str());
#else
    XBMC->DeleteFile(strCacheFile.c_str());
#endif
  }

  return ret == SERROR_OK;
}

SError SAPI::GetEvents(int iCurPlayType, int iEventActiveId, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  SError ret(SERROR_OK);

  memset(&params, 0, sizeof(params));
  params.action = WATCHDOG_GET_EVENTS;

  if (!sc_watchdog_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_watchdog_defaults failed", __FUNCTION__);
    return SERROR_API;
  }

  if ((param = sc_param_get(&params, "cur_play_type")))
    param->value.integer = iCurPlayType;

  if ((param = sc_param_get(&params, "event_active_id")))
    param->value.integer = iEventActiveId;

  ret = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return ret;
}
