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

#include "itv.h"

#include <string.h>

bool sc_itv_get_all_channels_defaults(sc_param_request_t *params) {
  params->param = NULL;

  return true;
}

bool sc_itv_get_ordered_list_defaults(sc_param_request_t *params) {
  sc_param_t *param;

  param = sc_param_create_string("genre", "*", false);
  param->first = param;

  param = sc_param_link(param, sc_param_create_integer("fav", 0, true));
  param = sc_param_link(param, sc_param_create_string("sortby", "number", true));
  param = sc_param_link(param, sc_param_create_integer("p", 0, false));

  params->param = param->first;

  return true;
}

bool sc_itv_create_link_defaults(sc_param_request_t *params) {
  sc_param_t *param;

  param = sc_param_create_string("cmd", "", true);
  param->first = param;

  param = sc_param_link(param, sc_param_create_string("forced_storage", "undefined", false));
  param = sc_param_link(param, sc_param_create_integer("disable_ad", 0, false));

  params->param = param->first;

  return true;
}

bool sc_itv_get_genres_defaults(sc_param_request_t *params) {
  params->param = NULL;

  return true;
}

bool sc_itv_get_epg_info_defaults(sc_param_request_t *params) {
  sc_param_t *param;

  param = sc_param_create_integer("period", 24, false);
  param->first = param;

  params->param = param->first;

  return true;
}

bool sc_itv_defaults(sc_param_request_t *params) {
  switch (params->action) {
    case ITV_GET_ALL_CHANNELS:
      return sc_itv_get_all_channels_defaults(params);
    case ITV_GET_ORDERED_LIST:
      return sc_itv_get_ordered_list_defaults(params);
    case ITV_CREATE_LINK:
      return sc_itv_create_link_defaults(params);
    case ITV_GET_GENRES:
      return sc_itv_get_genres_defaults(params);
    case ITV_GET_EPG_INFO:
      return sc_itv_get_epg_info_defaults(params);
  }

  return false;
}

bool sc_itv_prep_request(sc_param_request_t *params, sc_request_t *request) {
  const char *buffer;

  switch (params->action) {
    case ITV_GET_ALL_CHANNELS:
      buffer = "type=itv&action=get_all_channels&";
      break;
    case ITV_GET_ORDERED_LIST:
      buffer = "type=itv&action=get_ordered_list&";
      break;
    case ITV_CREATE_LINK:
      buffer = "type=itv&action=create_link&";
      break;
    case ITV_GET_GENRES:
      buffer = "type=itv&action=get_genres&";
      break;
    case ITV_GET_EPG_INFO:
      buffer = "type=itv&action=get_epg_info&";
      break;
  }

  request->method = "GET";

  if (buffer) {
    strncpy(request->query, buffer, strlen(buffer));
  }

  return true;
}

