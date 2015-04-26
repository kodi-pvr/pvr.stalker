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

#include "stb.h"

#include <string.h>

bool sc_stb_handshake_defaults(sc_param_request_t *params) {
  params->param = NULL;

  return true;
}

bool sc_stb_get_profile_defaults(sc_param_request_t *params) {
  sc_param_t *param;

  param = sc_param_create_string("stb_type", "MAG250", true);
  param->first = param;

  param = sc_param_link(param, sc_param_create_string("sn", "0000000000000", true));
  param = sc_param_link(param,
    sc_param_create_string("ver",
    "ImageDescription: 0.2.16-250; "
    "ImageDate: 18 Mar 2013 19:56:53 GMT+0200; "
    "PORTAL version: 4.9.9; "
    "API Version: JS API version: 328; "
    "STB API version: 134; "
    "Player Engine version: 0x560",
    true));
  param = sc_param_link(param, sc_param_create_string("device_id", "", false));
  param = sc_param_link(param, sc_param_create_string("device_id2", "", false));
  param = sc_param_link(param, sc_param_create_string("signature", "", false));
  param = sc_param_link(param, sc_param_create_boolean("not_valid_token", false, true));
  param = sc_param_link(param, sc_param_create_boolean("auth_second_step", false, true));
  param = sc_param_link(param, sc_param_create_boolean("hd", true, true));
  param = sc_param_link(param, sc_param_create_integer("num_banks", 1, true));
  param = sc_param_link(param, sc_param_create_integer("image_version", 216, true));
  param = sc_param_link(param, sc_param_create_string("hw_version", "1.7-BD-00", true));

  params->param = param->first;

  return true;
}

bool sc_stb_defaults(sc_param_request_t *params) {
  switch (params->action) {
    case STB_HANDSHAKE:
      return sc_stb_handshake_defaults(params);
    case STB_GET_PROFILE:
      return sc_stb_get_profile_defaults(params);
  }

  return false;
}

bool sc_stb_prep_request(sc_param_request_t *params, sc_request_t *request) {
  const char *buffer;

  switch (params->action) {
    case STB_HANDSHAKE:
      buffer = "type=stb&action=handshake&";
      break;
    case STB_GET_PROFILE:
      buffer = "type=stb&action=get_profile&";
      break;
  }

  request->method = "GET";

  if (buffer) {
    strncpy(request->query, buffer, strlen(buffer));
  }

  return true;
}
