/*
 *      Copyright (C) 2015, 2016  Jamal Edey
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

bool sc_itv_get_all_channels_defaults(sc_list_t *list) {
    return true;
}

bool sc_itv_get_ordered_list_defaults(sc_list_t *list) {
    sc_list_node_append(list, sc_list_node_create(sc_param_create_string("genre", "*", false)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("fav", 0, true)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_string("sortby", "number", true)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("p", 0, false)));

    return true;
}

bool sc_itv_create_link_defaults(sc_list_t *list) {
    sc_list_node_append(list, sc_list_node_create(sc_param_create_string("cmd", "", true)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_string("forced_storage", "undefined", false)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("disable_ad", 0, false)));

    return true;
}

bool sc_itv_get_genres_defaults(sc_list_t *list) {
    return true;
}

bool sc_itv_get_epg_info_defaults(sc_list_t *list) {
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("period", 24, false)));

    return true;
}

bool sc_itv_defaults(sc_param_params_t *params) {
    switch (params->action) {
        case ITV_GET_ALL_CHANNELS:
            return sc_itv_get_all_channels_defaults(params->list);
        case ITV_GET_ORDERED_LIST:
            return sc_itv_get_ordered_list_defaults(params->list);
        case ITV_CREATE_LINK:
            return sc_itv_create_link_defaults(params->list);
        case ITV_GET_GENRES:
            return sc_itv_get_genres_defaults(params->list);
        case ITV_GET_EPG_INFO:
            return sc_itv_get_epg_info_defaults(params->list);
        default:
            break;
    }

    return false;
}

bool sc_itv_prep_request(sc_param_params_t *params, sc_request_t *request) {
    sc_request_nameVal_t *paramPrev;
    sc_request_nameVal_t *param;

    paramPrev = request->params;
    while (paramPrev && paramPrev->next)
        paramPrev = paramPrev->next;

    param = sc_request_create_nameVal("type", "itv");

    if (!paramPrev) {
        param->first = param;
        request->params = paramPrev = param;
    } else {
        paramPrev = sc_request_link_nameVal(paramPrev, param);
    }

    switch (params->action) {
        case ITV_GET_ALL_CHANNELS:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "get_all_channels"));
            break;
        case ITV_GET_ORDERED_LIST:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "get_ordered_list"));
            break;
        case ITV_CREATE_LINK:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "create_link"));
            break;
        case ITV_GET_GENRES:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "get_genres"));
            break;
        case ITV_GET_EPG_INFO:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "get_epg_info"));
            break;
        default:
            break;
    }

    request->method = "GET";

    return true;
}
