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
#include <platform/os.h>

#include "client.h"

using namespace ADDON;

namespace SAPI
{
  bool Init()
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;
    size_t pos;
    std::string locationUrl;

    sock.SetURL(g_strServer);

    if (!sock.Execute(&resp_headers, &resp_body)) {
      XBMC->Log(LOG_ERROR, "%s: api init failed\n", __FUNCTION__);
      return false;
    }

    // xpcom.common.js > get_server_params()

    // check for location header
    if ((pos = resp_headers.find("Location: ")) != std::string::npos) {
      locationUrl = resp_headers.substr(pos + 10, resp_headers.find("\r\n", pos) - (pos + 10));
    }
    else {
      XBMC->Log(LOG_DEBUG, "%s: failed to get api endpoint from location header\n", __FUNCTION__);
      
      // convert to lower case
      std::transform(resp_body.begin(), resp_body.end(), resp_body.begin(), ::tolower);

      // check for meta refresh tag
      if ((pos = resp_body.find("url=")) != std::string::npos) {
        locationUrl = g_strServer + "/" + resp_body.substr(pos + 4, resp_body.find("\"", pos) - (pos + 4));
      }
      else {
        XBMC->Log(LOG_DEBUG, "%s: failed to get api endpoint from meta refresh tag\n", __FUNCTION__);

        // assume current url is the intended location
        XBMC->Log(LOG_DEBUG, "%s: assuming current url is the intended location\n", __FUNCTION__);
        locationUrl = g_strServer;
      }
    }

    if ((pos = locationUrl.find_last_of("/")) == std::string::npos || locationUrl.substr(pos - 2, 3).compare("/c/") != 0) {
      XBMC->Log(LOG_ERROR, "%s: failed to get api endpoint\n", __FUNCTION__);
      return false;
    }

    // strip tail from url path and set api endpoint and referer
    g_strApiBasePath = locationUrl.substr(0, pos - 1);
    g_api_endpoint = g_strApiBasePath + "server/load.php";
    g_referer = locationUrl.substr(0, pos + 1);

    XBMC->Log(LOG_DEBUG, "api endpoint: %s\n", g_api_endpoint.c_str());
    XBMC->Log(LOG_DEBUG, "referer: %s\n", g_referer.c_str());

    return true;
  }

  bool StalkerCall(HTTPSocket *sock, std::string *resp_headers, std::string *resp_body, Json::Value *parsed)
  {
    char buffer[256];
    size_t pos;
    Json::Reader reader;
    std::string cookie;

    snprintf(buffer, sizeof(buffer), "mac=%s; stb_lang=en; timezone=%s", g_strMac.c_str(), g_strTimeZone.c_str());
    cookie = buffer;
    //TODO url encode
    while ((pos = cookie.find(":")) != std::string::npos) {
      cookie.replace(pos, 1, "%3A");
    }
    while ((pos = cookie.find("/")) != std::string::npos) {
      cookie.replace(pos, 1, "%2F");
    }
    sock->AddHeader("Cookie", cookie);

    sock->AddHeader("Referer", g_referer);
    sock->AddHeader("X-User-Agent", "Model: MAG250; Link: WiFi");
    sock->AddHeader("Authorization", "Bearer " + g_token);

    if (!sock->Execute(resp_headers, resp_body)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    if (!reader.parse(*resp_body, *parsed)) {
      XBMC->Log(LOG_ERROR, "%s: parsing failed\n", __FUNCTION__);
      if (resp_body->compare(AUTHORIZATION_FAILED) == 0) {
        XBMC->Log(LOG_ERROR, "%s: authorization failed\n", __FUNCTION__);
      }
      return false;
    }

    return true;
  }

  bool Handshake(Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    sock.SetURL(g_api_endpoint + "?type=stb&action=handshake&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    g_token = (*parsed)["js"]["token"].asString();

    XBMC->Log(LOG_DEBUG, "token: %s\n", g_token.c_str());

    return true;
  }

  bool GetProfile(Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    sock.SetURL(g_api_endpoint + "?type=stb&action=get_profile"
      "&hd=1&ver=ImageDescription:%200.2.16-250;%20ImageDate:%2018%20Mar%202013%2019:56:53%20GMT+0200;%20PORTAL%20version:%204.9.9;%20API%20Version:%20JS%20API%20version:%20328;%20STB%20API%20version:%20134;%20Player%20Engine%20version:%200x560"
      "&num_banks=1&sn=0000000000000&stb_type=MAG250&image_version=216&auth_second_step=0&hw_version=1.7-BD-00&not_valid_token=0&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetAllChannels(Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    sock.SetURL(g_api_endpoint + "?type=itv&action=get_all_channels&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetOrderedList(uint32_t page, Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    sock.SetURL(g_api_endpoint + "?type=itv&action=get_ordered_list&genre=10&fav=0&sortby=number&p=" + std::to_string(page) +"&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool CreateLink(std::string &cmd, Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    std::string tmp = cmd;
    tmp.replace(tmp.find(" "), 1, "%20");

    sock.SetURL(g_api_endpoint + "?type=itv&action=create_link&cmd=" + tmp + "&forced_storage=undefined&disable_ad=0&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetGenres(Json::Value *parsed)
  {
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    sock.SetURL(g_api_endpoint + "?type=itv&action=get_genres&JsHttpRequest=1-xml&");

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetEPGInfo(uint32_t period, Json::Value *parsed)
  {
    char buffer[256];
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    snprintf(buffer, sizeof(buffer),
      "%s?type=itv&action=get_epg_info&period=%d&JsHttpRequest=1-xml&",
      g_api_endpoint.c_str(), period);

    sock.SetURL(std::string(buffer));

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetWeek(Json::Value *parsed)
  {
    char buffer[256];
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    snprintf(buffer, sizeof(buffer),
      "%s?type=epg&action=get_week&JsHttpRequest=1-xml&",
      g_api_endpoint.c_str());

    sock.SetURL(std::string(buffer));

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetSimpleDataTable(uint32_t channelId, std::string &date, uint32_t page, Json::Value *parsed)
  {
    char buffer[256];
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    snprintf(buffer, sizeof(buffer),
      "%s?type=epg&action=get_simple_data_table&ch_id=%d&date=%s&p=%d&JsHttpRequest=1-xml&",
      g_api_endpoint.c_str(), channelId, date.c_str(), page);

    sock.SetURL(std::string(buffer));

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }

  bool GetDataTable(uint32_t channelId, std::string &from, std::string &to, uint32_t page, Json::Value *parsed)
  {
    char buffer[256];
    HTTPSocket sock;
    std::string resp_headers;
    std::string resp_body;

    snprintf(buffer, sizeof(buffer),
      "%s?type=epg&action=get_data_table&ch_id=%d&from=%s&to=%s&p=%d&JsHttpRequest=1-xml&",
      g_api_endpoint.c_str(), channelId, from.c_str(), to.c_str(), page);

    sock.SetURL(std::string(buffer));

    if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
      return false;
    }

    return true;
  }
}
