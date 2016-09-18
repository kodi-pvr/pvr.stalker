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

#include "list.h"

#include <stdlib.h>
#include <string.h>

sc_list_t *sc_list_create() {
    sc_list_t *list;

    list = (sc_list_t *) malloc(sizeof(sc_list_t));
    memset(list, 0, sizeof(*list));

    return list;
}

sc_list_node_t *sc_list_node_create(void *data) {
    sc_list_node_t *node;

    node = (sc_list_node_t *) malloc(sizeof(sc_list_node_t));
    memset(node, 0, sizeof(*node));
    node->data = data;

    return node;
}

sc_list_node_t *sc_list_node_link(sc_list_t *list, sc_list_node_t *a, sc_list_node_t *b) {
    b->prev = a;
    a->next = b;
    list->last = b;

    return b;
}

sc_list_node_t *sc_list_node_append(sc_list_t *list, sc_list_node_t *node) {
    if (!list->first) {
        list->first = node;
        list->last = node;
        return node;
    }
    return sc_list_node_link(list, list->last, node);
}

sc_list_node_t *sc_list_node_unlink(sc_list_t *list, sc_list_node_t *node) {
    sc_list_node_t *prev = NULL;
    sc_list_node_t *next = NULL;

    prev = node->prev;
    next = node->next;

    if (node == list->first) {
        list->first = next;
    }

    if (node == list->last) {
        list->last = prev;
    }

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    node->prev = NULL;
    node->next = NULL;

    return next;
}

void sc_list_node_free(sc_list_node_t **node, bool free_data) {
    if (!node) return;
    if (*node) {
        if (free_data && (*node)->data) {
            free((*node)->data);
        }

        (*node)->data = NULL;
        (*node)->prev = NULL;
        (*node)->next = NULL;

        free(*node);
    }
    *node = NULL;
}

void sc_list_free(sc_list_t **list, bool free_node_data) {
    if (!list) return;
    if (*list) {
        sc_list_node_t *node;
        sc_list_node_t *next;

        node = (*list)->first;
        while (node) {
            next = node->next;
            sc_list_node_free(&node, free_node_data);
            node = next;
        }

        (*list)->first = NULL;
        (*list)->last = NULL;

        free(*list);
    }
    *list = NULL;
}
