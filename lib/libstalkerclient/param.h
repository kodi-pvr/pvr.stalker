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

#ifndef PARAM_H
#define PARAM_H

#include <stdbool.h>

#include "action.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SC_STRING,
    SC_INTEGER,
    SC_BOOLEAN
} sc_param_type_t;

typedef struct sc_param {
    const char *name;
    sc_param_type_t type;
    union {
        char *string;
        int integer;
        bool boolean;
    } value;
    bool required;
} sc_param_t;

typedef struct {
    sc_action_t action;
    sc_list_t *list;
} sc_param_params_t;

sc_param_params_t *sc_param_params_create(sc_action_t action);

sc_param_t *sc_param_create(const char *name, sc_param_type_t type, bool required);

sc_param_t *sc_param_create_string(const char *name, char *value, bool required);

sc_param_t *sc_param_create_integer(const char *name, int value, bool required);

sc_param_t *sc_param_create_boolean(const char *name, bool value, bool required);

sc_param_t *sc_param_get(sc_param_params_t *params, const char *name);

sc_param_t *sc_param_get2(sc_param_params_t *params, const char *name, sc_list_node_t **param_node);

sc_param_t *sc_param_copy(sc_param_t *param);

void sc_param_free(sc_param_t **param);

void sc_param_params_free(sc_param_params_t **params);

#ifdef __cplusplus
}
#endif

#endif /* PARAM_H */
