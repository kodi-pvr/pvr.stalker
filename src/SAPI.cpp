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
 
#include "platform/os.h"

#include "libstalkerclient/param.h"
#include "libstalkerclient/stb.h"
#include "libstalkerclient/itv.h"
#include "libstalkerclient/util.h"
#include "client.h"

using namespace ADDON;

namespace SAPI
{
  bool Init()
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
    
    bool isHttp;
    Request request;
    Response response;
    HTTPSocket *sock = NULL;
    size_t pos;
    std::string strRealServer;
    
    isHttp = g_strServer.find("http://") == 0;
    
    sock = isHttp
      ? new HTTPSocketRaw
      : new HTTPSocket;
    
    request.url = g_strServer;
    
    if (!sock->Execute(&request, &response) || (!isHttp && response.body.empty())) {
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
        if ((pos = response.body.find("url=")) != std::string::npos) {
          strRealServer = g_strServer + "/" + response.body.substr(pos + 4, response.body.find("\"", pos) - (pos + 4));
        }
        else {
          XBMC->Log(LOG_DEBUG, "%s: failed to get api endpoint from meta refresh tag", __FUNCTION__);
        }
      }
    }
    
    if (strRealServer.empty()) {
      // assume current url is the intended location
      XBMC->Log(LOG_DEBUG, "%s: assuming current url is the intended location", __FUNCTION__);
      strRealServer = g_strServer;
    }
    
    // xpcom.common.js > get_server_params()
    if ((pos = strRealServer.find_last_of("/")) == std::string::npos || strRealServer.substr(pos - 2, 3).compare("/c/") != 0) {
      XBMC->Log(LOG_ERROR, "%s: failed to get api endpoint", __FUNCTION__);
      return false;
    }
    
    // strip tail from url path and set api endpoint and referer
    g_strApiBasePath = strRealServer.substr(0, pos - 1);
    g_api_endpoint = g_strApiBasePath + "server/load.php";
    g_referer = strRealServer.substr(0, pos + 1);

    XBMC->Log(LOG_DEBUG, "api endpoint: %s", g_api_endpoint.c_str());
    XBMC->Log(LOG_DEBUG, "referer: %s", g_referer.c_str());

    return true;
  }

  bool StalkerCall(sc_identity_t *identity, sc_param_request_t *params, Response *response, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_request_t scRequest;
    sc_request_header_t *scHeader;
    Request request;
    HTTPSocket sock;
    size_t pos;
    Json::Reader reader;

    memset(&scRequest, 0, sizeof(scRequest));
    if (!sc_request_build(identity, params, &scRequest)) {
      XBMC->Log(LOG_ERROR, "sc_request_build failed");
    }

    scHeader = scRequest.headers;
    while (scHeader) {
      std::string strValue;

      strValue = scHeader->value;

      //TODO url encode
      while ((pos = strValue.find(":")) != std::string::npos) {
        strValue.replace(pos, 1, "%3A");
      }
      while ((pos = strValue.find("/")) != std::string::npos) {
        strValue.replace(pos, 1, "%2F");
      }

      request.AddHeader(scHeader->name, strValue);

      scHeader = scHeader->next;
    }

    request.AddHeader("Referer", g_referer);
    request.AddHeader("X-User-Agent", "Model: MAG250; Link: WiFi");

    //TODO url encode
    std::string query;
    query = scRequest.query;
    while ((pos = query.find(" ")) != std::string::npos) {
      query.replace(pos, 1, "%20");
    }

    request.url = g_api_endpoint + "?" + query;

    sc_request_free_headers(scRequest.headers);

    if (!sock.Execute(&request, response)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    if (!reader.parse(response->body, *parsed)) {
      XBMC->Log(LOG_ERROR, "%s: parsing failed", __FUNCTION__);
      if (response->body.compare(AUTHORIZATION_FAILED) == 0) {
        XBMC->Log(LOG_ERROR, "%s: authorization failed", __FUNCTION__);
      }
      return false;
    }

    return true;
  }

  bool Handshake(sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    sc_param_t *param;
    std::string resp_headers;
    std::string resp_body;

    memset(&params, 0, sizeof(params));
    params.action = STB_HANDSHAKE;

    if (!sc_stb_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_stb_defaults failed", __FUNCTION__);
      return false;
    }
    
    if (identity->auth_token && strlen(identity->auth_token) > 0
      && (param = sc_param_get(&params, "token")))
    {
      free(param->value.string);
      param->value.string = sc_util_strcpy((char *)identity->auth_token);
    }

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool GetProfile(sc_identity_t *identity, bool bAuthTokenNotValid, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    sc_param_t *param;
    std::string resp_headers;
    std::string resp_body;

    memset(&params, 0, sizeof(params));
    params.action = STB_GET_PROFILE;

    if (!sc_stb_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_stb_defaults failed", __FUNCTION__);
      return false;
    }
    
    if ((param = sc_param_get(&params, "not_valid_token")))
      param->value.boolean = bAuthTokenNotValid;

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool GetAllChannels(sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    Response response;

    memset(&params, 0, sizeof(params));
    params.action = ITV_GET_ALL_CHANNELS;

    if (!sc_itv_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
      return false;
    }

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool GetOrderedList(std::string &genre, uint32_t page, sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    sc_param_t *param;
    Response response;

    memset(&params, 0, sizeof(params));
    params.action = ITV_GET_ORDERED_LIST;

    if (!sc_itv_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
      return false;
    }

    if ((param = sc_param_get(&params, "genre"))) {
      free(param->value.string);
      param->value.string = sc_util_strcpy((char *)genre.c_str());
    }

    if ((param = sc_param_get(&params, "p"))) {
      param->value.integer = page;
    }

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool CreateLink(std::string &cmd, sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    sc_param_t *param;
    Response response;

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

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool GetGenres(sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    Response response;

    memset(&params, 0, sizeof(params));
    params.action = ITV_GET_GENRES;

    if (!sc_itv_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
      return false;
    }

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }

  bool GetEPGInfo(uint32_t period, sc_identity_t *identity, Json::Value *parsed)
  {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    sc_param_request_t params;
    sc_param_t *param;
    Response response;

    memset(&params, 0, sizeof(params));
    params.action = ITV_GET_EPG_INFO;

    if (!sc_itv_defaults(&params)) {
      XBMC->Log(LOG_ERROR, "%s: sc_itv_defaults failed", __FUNCTION__);
      return false;
    }

    if ((param = sc_param_get(&params, "period"))) {
      param->value.integer = period;
    }

    if (!StalkerCall(identity, &params, &response, parsed)) {
      sc_param_free_params(params.param);
      XBMC->Log(LOG_ERROR, "%s: api call failed", __FUNCTION__);
      return false;
    }

    sc_param_free_params(params.param);

    return true;
  }
}
