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
#include <stdint.h>
#include <json/json.h>

#include "libstalkerclient/identity.h"
#include "libstalkerclient/request.h"
#include "HTTPSocket.h"
#include "SData.h"

#define AUTHORIZATION_FAILED "Authorization failed."

class SAPI
{
public:
  static bool Init();
  static SError StalkerCall(sc_identity_t &identity, sc_param_request_t &params, Response &response, Json::Value &parsed,
    bool bCache = false, std::string strCacheFile = "", uint32_t cacheExpiry = 0);
  static bool Handshake(sc_identity_t &identity, Json::Value &parsed);
  static bool GetProfile(sc_identity_t &identity, bool bAuthSecondStep, Json::Value &parsed);
  static bool DoAuth(sc_identity_t &identity, Json::Value &parsed);
  static bool GetAllChannels(sc_identity_t &identity, Json::Value &parsed);
  static bool GetOrderedList(int iGenre, int iPage, sc_identity_t &identity, Json::Value &parsed);
  static bool CreateLink(std::string &cmd, sc_identity_t &identity, Json::Value &parsed);
  static bool GetGenres(sc_identity_t &identity, Json::Value &parsed);
  static bool GetEPGInfo(int iPeriod, sc_identity_t &identity, Json::Value &parsed,
    bool bCache, uint32_t cacheExpiry);
  static SError GetEvents(int iCurPlayType, int iEventActiveId, sc_identity_t &identity, Json::Value &parsed);
};
