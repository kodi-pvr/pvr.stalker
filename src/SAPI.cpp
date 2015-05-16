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
 
#include "platform/os.h"

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
  bool isHttp;
  Request request;
  Response response;
  HTTPSocket *sock = NULL;
  size_t pos;
  std::string strRealServer;

  if (g_strServer.find("://") == std::string::npos)
    strServer = "http://";
  
  strServer += g_strServer;
  isHttp = strServer.find("://") == 0;

  sock = isHttp
    ? new HTTPSocketRaw(g_iConnectionTimeout)
    : new HTTPSocket(g_iConnectionTimeout);

  request.url = strServer;

  if (!sock->Execute(request, response) || (!isHttp && response.body.empty())) {
    XBMC->Log(LOG_ERROR, "%s: api init failed", __FUNCTION__);
    return false;
  }

  if (isHttp) {
    // check for location header
    if ((pos = response.headers.find("Location: ")) != std::string::npos) {
      strRealServer = response.headers.substr(pos + 10, response.headers.find("\r\n", pos) - (pos + 10));
    }
    else {
      XBMC->Log(LOG_DEBUG, "%s: failed to get api endpoint from location header", __FUNCTION__);

      // convert to lower case
      std::transform(response.body.begin(), response.body.end(), response.body.begin(), ::tolower);

      // check for meta refresh tag
      if ((pos = response.body.find("url=")) != std::string::npos)
        strRealServer = strServer + "/" + response.body.substr(pos + 4, response.body.find("\"", pos) - (pos + 4));
      else
        XBMC->Log(LOG_DEBUG, "%s: failed to get api endpoint from meta refresh tag", __FUNCTION__);
    }
  }

  if (strRealServer.empty()) {
    // assume current url is the intended location
    XBMC->Log(LOG_DEBUG, "%s: assuming current url is the intended location", __FUNCTION__);
    strRealServer = strServer;
  }

  // xpcom.common.js > get_server_params()
  if ((pos = strRealServer.find_last_of("/")) == std::string::npos || strRealServer.substr(pos - 2, 3).compare("/c/") != 0) {
    XBMC->Log(LOG_ERROR, "%s: failed to get api endpoint", __FUNCTION__);
    return false;
  }

  // strip tail from url path and set api endpoint and referer
  g_strApiBasePath = strRealServer.substr(0, pos - 1);
  g_strApiEndpoint = g_strApiBasePath + "server/load.php";
  g_strReferer = strRealServer.substr(0, pos + 1);

  XBMC->Log(LOG_DEBUG, "api_endpoint=%s", g_strApiEndpoint.c_str());
  XBMC->Log(LOG_DEBUG, "referer=%s", g_strReferer.c_str());

  return true;
}

bool SAPI::StalkerCall(sc_identity_t &identity, sc_param_request_t &params, Response &response, Json::Value &parsed)
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
    request.AddHeader(scNameVal->name, scNameVal->value);

    scNameVal = scNameVal->next;
  }

  request.AddHeader("Referer", g_strReferer);
  request.AddHeader("X-User-Agent", "Model: MAG250; Link: WiFi");

  sc_request_free_nameVals(scRequest.headers);

  oss << g_strApiEndpoint << "?";
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

  if (!sock.Execute(request, response)) {
    XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
    return false;
  }

  if (!reader.parse(response.body, parsed)) {
    XBMC->Log(LOG_ERROR, "%s: parsing failed", __FUNCTION__);
    if (response.body.compare(AUTHORIZATION_FAILED) == 0) {
      XBMC->Log(LOG_ERROR, "%s: authorization failed", __FUNCTION__);
    }
    return false;
  }

  return true;
}

bool SAPI::Handshake(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

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

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetProfile(sc_identity_t &identity, bool bAuthSecondStep, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

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
  
  if (strlen(identity.device_id) > 0
    && (param = sc_param_get(&params, "device_id")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id);
  }
  
  if (strlen(identity.device_id2) > 0
    && (param = sc_param_get(&params, "device_id2")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id2);
  }
  
  if (strlen(identity.signature) > 0
    && (param = sc_param_get(&params, "signature")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.signature);
  }

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::DoAuth(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

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
  
  if (strlen(identity.device_id) > 0
    && (param = sc_param_get(&params, "device_id")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id);
  }
  
  if (strlen(identity.device_id2) > 0
    && (param = sc_param_get(&params, "device_id2")))
  {
    free(param->value.string);
    param->value.string = sc_util_strcpy(identity.device_id2);
  }

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetAllChannels(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  Response response;
  bool result(true);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_ALL_CHANNELS;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetOrderedList(int iGenre, int iPage, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

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

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::CreateLink(std::string &cmd, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

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

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetGenres(sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  Response response;
  bool result(true);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_GENRES;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetEPGInfo(int iPeriod, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

  memset(&params, 0, sizeof(params));
  params.action = ITV_GET_EPG_INFO;

  if (!sc_itv_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "period"))) {
    param->value.integer = iPeriod;
  }

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}

bool SAPI::GetEvents(int iCurPlayType, int iEventActiveId, sc_identity_t &identity, Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_param_request_t params;
  sc_param_t *param;
  Response response;
  bool result(true);

  memset(&params, 0, sizeof(params));
  params.action = WATCHDOG_GET_EVENTS;

  if (!sc_watchdog_defaults(&params)) {
    XBMC->Log(LOG_ERROR, "%s: sc_watchdog_defaults failed", __FUNCTION__);
    return false;
  }

  if ((param = sc_param_get(&params, "cur_play_type")))
    param->value.integer = iCurPlayType;

  if ((param = sc_param_get(&params, "event_active_id")))
    param->value.integer = iEventActiveId;

  result = StalkerCall(identity, params, response, parsed);

  sc_param_free_params(params.param);

  return result;
}
