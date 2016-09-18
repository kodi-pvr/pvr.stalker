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

#include "watchdog.h"

bool sc_watchdog_get_events_defaults(sc_list_t *list) {
    sc_list_node_append(list, sc_list_node_create(sc_param_create_boolean("init", false, true)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("cur_play_type", 0, true)));
    sc_list_node_append(list, sc_list_node_create(sc_param_create_integer("event_active_id", 0, true)));

    return true;
}

bool sc_watchdog_defaults(sc_param_params_t *params) {
    switch (params->action) {
        case WATCHDOG_GET_EVENTS:
            return sc_watchdog_get_events_defaults(params->list);
        default:
            break;
    }

    return false;
}

bool sc_watchdog_prep_request(sc_param_params_t *params, sc_request_t *request) {
    sc_request_nameVal_t *paramPrev;
    sc_request_nameVal_t *param;

    paramPrev = request->params;
    while (paramPrev && paramPrev->next)
        paramPrev = paramPrev->next;

    param = sc_request_create_nameVal("type", "watchdog");

    if (!paramPrev) {
        param->first = param;
        request->params = paramPrev = param;
    } else {
        paramPrev = sc_request_link_nameVal(paramPrev, param);
    }

    switch (params->action) {
        case WATCHDOG_GET_EVENTS:
            sc_request_link_nameVal(paramPrev, sc_request_create_nameVal("action", "get_events"));
            break;
        default:
            break;
    }

    request->method = "GET";

    return true;
}
