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

#include "param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

sc_param_params_t *sc_param_params_create(sc_action_t action) {
    sc_param_params_t *params;

    params = (sc_param_params_t *) malloc(sizeof(sc_param_params_t));
    memset(params, 0, sizeof(*params));

    params->action = action;
    params->list = sc_list_create();

    return params;
}

sc_param_t *sc_param_create(const char *name, sc_param_type_t type, bool required) {
    sc_param_t *param;

    param = (sc_param_t *) malloc(sizeof(sc_param_t));
    memset(param, 0, sizeof(*param));

    param->name = name;
    param->type = type;
    param->required = required;

    return param;
}

sc_param_t *sc_param_create_string(const char *name, char *value, bool required) {
    sc_param_t *param;

    param = sc_param_create(name, SC_STRING, required);
    param->value.string = sc_util_strcpy(value);

    return param;
}

sc_param_t *sc_param_create_integer(const char *name, int value, bool required) {
    sc_param_t *param;

    param = sc_param_create(name, SC_INTEGER, required);
    param->value.integer = value;

    return param;
}

sc_param_t *sc_param_create_boolean(const char *name, bool value, bool required) {
    sc_param_t *param;

    param = sc_param_create(name, SC_BOOLEAN, required);
    param->value.boolean = value;

    return param;
}

sc_param_t *sc_param_get(sc_param_params_t *params, const char *name) {
    return sc_param_get2(params, name, NULL);
}

sc_param_t *sc_param_get2(sc_param_params_t *params, const char *name, sc_list_node_t **param_node) {
    sc_list_node_t *node;
    sc_param_t *param;

    node = params->list->first;
    while (node) {
        param = (sc_param_t *) node->data;
        if (!strcmp(param->name, name)) {
            if (param_node)
                *param_node = node;
            return param;
        }
        node = node->next;
    }

    return NULL;
}

sc_param_t *sc_param_copy(sc_param_t *param) {
    sc_param_t *copy;

    copy = sc_param_create(param->name, param->type, param->required);

    switch (param->type) {
        case SC_STRING:
            copy->value.string = sc_util_strcpy(param->value.string);
            break;
        case SC_INTEGER:
            copy->value.integer = param->value.integer;
            break;
        case SC_BOOLEAN:
            copy->value.boolean = param->value.boolean;
            break;
    }

    return copy;
}

void sc_param_free(sc_param_t **param) {
    if (!param) return;
    if (*param) {
        if ((*param)->type == SC_STRING) {
            free((*param)->value.string);
        }
        free(*param);
    }
    *param = NULL;
}

void sc_param_params_free(sc_param_params_t **params) {
    if (!params) return;
    if (*params) {
        if ((*params)->list) {
            sc_list_node_t *node;

            node = (*params)->list->first;
            while (node) {
                sc_param_free((sc_param_t **) &node->data);
                node = node->next;
            }

            sc_list_free(&(*params)->list, false);
        }

        free(*params);
    }
    *params = NULL;
}
