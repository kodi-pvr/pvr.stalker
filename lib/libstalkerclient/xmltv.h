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

#ifndef XMLTV_H
#define XMLTV_H

#include <stdbool.h>
#include <time.h>

#include <libxml/xmlreader.h>

#include "list.h"

#if defined(_MSC_VER) && _MSC_VER >= 1900
#define timezone _timezone
#endif

#define SC_XMLTV_CHANNEL_NAME     "channel"
#define SC_XMLTV_CHANNEL_DEPTH    1

#define SC_XMLTV_PROGRAMME_NAME   "programme"
#define SC_XMLTV_PROGRAMME_DEPTH  1

#define SC_XMLTV_CREDIT_NAME      "credits"
#define SC_XMLTV_CREDIT_DEPTH     2

#ifdef __cplusplus
extern "C" {
#endif

enum sc_xmltv_strct {
    SC_XMLTV_CHANNEL,
    SC_XMLTV_PROGRAMME,
    SC_XMLTV_CREDIT
};

typedef struct sc_xmltv_channel {
    char *id_;
    sc_list_t *display_names;
    sc_list_t *programmes;
} sc_xmltv_channel_t;

typedef struct sc_xmltv_programme {
    time_t start;
    time_t stop;
    char *channel;
    char *title;
    char *sub_title;
    char *desc;
    sc_list_t *credits;
    char *date;
    sc_list_t *categories;
    int episode_num;
    time_t previously_shown;
    char *star_rating;
    char *icon;
} sc_xmltv_programme_t;

typedef enum sc_xmltv_credit_type {
    SC_XMLTV_CREDIT_TYPE_UNKNOWN,
    SC_XMLTV_CREDIT_TYPE_ACTOR,
    SC_XMLTV_CREDIT_TYPE_DIRECTOR,
    SC_XMLTV_CREDIT_TYPE_GUEST,
    SC_XMLTV_CREDIT_TYPE_PRESENTER,
    SC_XMLTV_CREDIT_TYPE_PRODUCER,
    SC_XMLTV_CREDIT_TYPE_WRITER
} sc_xmltv_credit_type_t;

typedef struct sc_xmltv_credit {
    sc_xmltv_credit_type_t type;
    char *name;
} sc_xmltv_credit_t;

void *sc_xmltv_create(enum sc_xmltv_strct type);

void sc_xmltv_list_free(enum sc_xmltv_strct type, sc_list_t **list);

void sc_xmltv_free(enum sc_xmltv_strct type, void *strct);

time_t sc_xmltv_to_unix_time(const char *str_time);

bool sc_xmltv_check_current_reader_node(xmlTextReaderPtr reader, int node_type, const char *node_name, int node_depth);

sc_xmltv_channel_t *sc_xmltv_parse_channel(xmlTextReaderPtr reader);

sc_xmltv_programme_t *sc_xmltv_parse_programme(xmlTextReaderPtr reader);

sc_list_t *sc_xmltv_parse(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* XMLTV_H */
