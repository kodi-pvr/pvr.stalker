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

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sc_list_node;

typedef struct sc_list_node {
    void *data;
    struct sc_list_node *prev;
    struct sc_list_node *next;
} sc_list_node_t;

typedef struct sc_list {
    sc_list_node_t *first;
    sc_list_node_t *last;
} sc_list_t;

sc_list_t *sc_list_create();

sc_list_node_t *sc_list_node_create(void *data);

sc_list_node_t *sc_list_node_link(sc_list_t *list, sc_list_node_t *a, sc_list_node_t *b);

sc_list_node_t *sc_list_node_unlink(sc_list_t *list, sc_list_node_t *node);

sc_list_node_t *sc_list_node_append(sc_list_t *list, sc_list_node_t *node);

void sc_list_node_free(sc_list_node_t **node, bool free_data);

void sc_list_free(sc_list_t **list, bool free_node_data);

#ifdef __cplusplus
}
#endif

#endif /* LIST_H */
