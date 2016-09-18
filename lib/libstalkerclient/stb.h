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

#ifndef STB_H
#define STB_H

#include <stdbool.h>

#include "param.h"
#include "request.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool store_auth_data_on_stb;
    int status;
    char msg[SC_STR_LENGTH];
    char block_msg[SC_STR_LENGTH];
    int watchdog_timeout;
    double timeslot;
} sc_stb_profile_t;

bool sc_stb_handshake_defaults(sc_list_t *list);

bool sc_stb_get_profile_defaults(sc_list_t *list);

bool sc_stb_do_auth_defaults(sc_list_t *list);

bool sc_stb_defaults(sc_param_params_t *params);

bool sc_stb_prep_request(sc_param_params_t *params, sc_request_t *request);

void sc_stb_profile_defaults(sc_stb_profile_t *profile);

#ifdef __cplusplus
}
#endif

#endif /* STB_H */
