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

#define AUTHORIZATION_FAILED "Authorization failed."

namespace SAPI
{
  bool Init();
  bool StalkerCall(sc_identity_t *identity, sc_param_request_t *params, std::string *resp_headers, std::string *resp_body, Json::Value *parsed);
  bool Handshake(sc_identity_t *identity, Json::Value *parsed);
  bool GetProfile(sc_identity_t *identity, Json::Value *parsed);
  bool GetAllChannels(sc_identity_t *identity, Json::Value *parsed);
  bool GetOrderedList(std::string &genre, uint32_t page, sc_identity_t *identity, Json::Value *parsed);
  bool CreateLink(std::string &cmd, sc_identity_t *identity, Json::Value *parsed);
  bool GetGenres(sc_identity_t *identity, Json::Value *parsed);
  bool GetEPGInfo(uint32_t period, sc_identity_t *identity, Json::Value *parsed);
};
