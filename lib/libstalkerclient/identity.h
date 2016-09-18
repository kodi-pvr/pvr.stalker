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

#ifndef IDENTITY_H
#define IDENTITY_H

#include <stdbool.h>

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char mac[SC_STR_LENGTH];
    char lang[SC_STR_LENGTH];
    char time_zone[SC_STR_LENGTH];
    char token[SC_STR_LENGTH];
    bool valid_token;
    char login[SC_STR_LENGTH];
    char password[SC_STR_LENGTH];
    char serial_number[SC_STR_LENGTH];
    char device_id[SC_STR_LENGTH];
    char device_id2[SC_STR_LENGTH];
    char signature[SC_STR_LENGTH];
} sc_identity_t;

void sc_identity_defaults(sc_identity_t *identity);

#ifdef __cplusplus
}
#endif

#endif /* IDENTITY_H */
