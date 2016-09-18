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

#include "xmltv.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include "util.h"

void *sc_xmltv_create(enum sc_xmltv_strct type) {
    size_t size = 0;
    void *strct = NULL;

    switch (type) {
        case SC_XMLTV_CHANNEL:
            size = sizeof(sc_xmltv_channel_t);
            break;
        case SC_XMLTV_PROGRAMME:
            size = sizeof(sc_xmltv_programme_t);
            break;
        case SC_XMLTV_CREDIT:
            size = sizeof(sc_xmltv_credit_t);
            break;
    }

    if (size == 0)
        return NULL;

    strct = malloc(size);
    memset(strct, 0, size);

    switch (type) {
        case SC_XMLTV_CHANNEL: {
            sc_xmltv_channel_t *c = (sc_xmltv_channel_t *) strct;
            c->display_names = sc_list_create();
            c->programmes = sc_list_create();
            break;
        }
        case SC_XMLTV_PROGRAMME: {
            sc_xmltv_programme_t *p = (sc_xmltv_programme_t *) strct;
            p->credits = sc_list_create();
            p->categories = sc_list_create();
            break;
        }
        default:
            break;
    }

    return strct;
}

void sc_xmltv_list_free(enum sc_xmltv_strct type, sc_list_t **list) {
    sc_list_node_t *node;
    sc_list_node_t *next;

    node = (*list)->first;
    while (node) {
        next = node->next;
        sc_xmltv_free(type, node->data);
        sc_list_node_free(&node, false);
        node = next;
    }

    (*list)->first = NULL;
    (*list)->last = NULL;

    free(*list);
    *list = NULL;
}

void sc_xmltv_free(enum sc_xmltv_strct type, void *strct) {
    //  if (!strct) return;

    switch (type) {
        case SC_XMLTV_CHANNEL: {
            sc_xmltv_channel_t *c = (sc_xmltv_channel_t *) strct;

            if (c->id_) free(c->id_);
            c->id_ = NULL;

            sc_list_free(&c->display_names, true);
            sc_xmltv_list_free(SC_XMLTV_PROGRAMME, &c->programmes);

            break;
        }
        case SC_XMLTV_PROGRAMME: {
            sc_xmltv_programme_t *p = (sc_xmltv_programme_t *) strct;

            p->start = 0;
            p->stop = 0;
            if (p->channel) free(p->channel);
            p->channel = NULL;
            if (p->title) free(p->title);
            p->title = NULL;
            if (p->sub_title) free(p->sub_title);
            p->sub_title = NULL;
            if (p->desc) free(p->desc);
            p->desc = NULL;

            sc_xmltv_list_free(SC_XMLTV_CREDIT, &p->credits);

            if (p->date) free(p->date);
            p->date = NULL;

            sc_list_free(&p->categories, true);

            p->episode_num = 0;
            p->previously_shown = 0;
            if (p->star_rating) free(p->star_rating);
            p->star_rating = NULL;
            if (p->icon) free(p->icon);
            p->icon = NULL;

            break;
        }
        case SC_XMLTV_CREDIT: {
            sc_xmltv_credit_t *c = (sc_xmltv_credit_t *) strct;

            c->type = SC_XMLTV_CREDIT_TYPE_UNKNOWN;
            if (c->name) free(c->name);
            c->name = NULL;

            break;
        }
    }

    free(strct);
//    strct = NULL;
}

time_t sc_xmltv_to_unix_time(const char *str_time) {
    if (!str_time) return 0;

    struct tm time_info;
    int offset = 0;
    time_t time;

    sscanf(str_time, "%04d%02d%02d%02d%02d%02d",
           &time_info.tm_year, &time_info.tm_mon, &time_info.tm_mday,
           &time_info.tm_hour, &time_info.tm_min, &time_info.tm_sec);

    time_info.tm_year -= 1900;
    time_info.tm_mon -= 1;
    time_info.tm_isdst = -1;

    if (strlen(str_time) == 20) {
        char str_sign[2] = {0};
        int hour_offset = 0;
        int min_offset = 0;

        sscanf(&str_time[15], "%01s%02d%02d",
               str_sign, &hour_offset, &min_offset);

        hour_offset *= 3600;
        min_offset *= 60;
        offset = hour_offset + min_offset;
        if (strcmp(str_sign, "-") == 0) {
            offset *= -1;
        }
    }

    time = mktime(&time_info);
    if (time_info.tm_isdst > 0) {
        time += 3600;
    }
    time += offset - timezone;

    return time;
}

bool sc_xmltv_get_reader_value(xmlTextReaderPtr reader, char **dst) {
    bool ret = false;
    xmlChar *val = xmlTextReaderValue(reader);
    if (val) {
        *dst = sc_util_strcpy((char *) val);
        ret = true;
    }
    xmlFree(val);
    return ret;
}

bool sc_xmltv_get_reader_element_value(xmlTextReaderPtr reader, char **dst) {
    if (xmlTextReaderRead(reader) == 1 && xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
        return sc_xmltv_get_reader_value(reader, dst);
    }
    return false;
}

bool sc_xmltv_get_reader_property_value(xmlTextReaderPtr reader, const char *prop, char **dst) {
    if (xmlTextReaderMoveToAttribute(reader, (const xmlChar *) prop) == 1) {
        return sc_xmltv_get_reader_value(reader, dst);
    }
    return false;
}

bool sc_xmltv_check_current_reader_node(xmlTextReaderPtr reader, int node_type, const char *node_name, int node_depth) {
    xmlChar *name = NULL;
    bool result;
    name = xmlTextReaderName(reader);
    result = !xmlTextReaderIsEmptyElement(reader) &&
             xmlTextReaderNodeType(reader) == node_type &&
             (!xmlStrcmp(name, (const xmlChar *) node_name)) &&
             xmlTextReaderDepth(reader) == node_depth;
    xmlFree(name);
    return result;
}

void sc_xmltv_parse_credits(xmlTextReaderPtr reader, sc_list_t **list) {
    sc_xmltv_credit_t *cred = NULL;
    int ret;
    xmlChar *val;
    sc_xmltv_credit_type_t type;
    sc_list_node_t *node = NULL;

    for (ret = xmlTextReaderRead(reader); ret == 1; ret = xmlTextReaderRead(reader)) {
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_END_ELEMENT, SC_XMLTV_CREDIT_NAME,
                                               SC_XMLTV_CREDIT_DEPTH)) {
            break;
        }

        val = xmlTextReaderName(reader);
        type = SC_XMLTV_CREDIT_TYPE_UNKNOWN;
        if ((!xmlStrcmp(val, (const xmlChar *) "actor"))) type = SC_XMLTV_CREDIT_TYPE_ACTOR;
        if ((!xmlStrcmp(val, (const xmlChar *) "director"))) type = SC_XMLTV_CREDIT_TYPE_DIRECTOR;
        if ((!xmlStrcmp(val, (const xmlChar *) "guest"))) type = SC_XMLTV_CREDIT_TYPE_GUEST;
        if ((!xmlStrcmp(val, (const xmlChar *) "presenter"))) type = SC_XMLTV_CREDIT_TYPE_PRESENTER;
        if ((!xmlStrcmp(val, (const xmlChar *) "producer"))) type = SC_XMLTV_CREDIT_TYPE_PRODUCER;
        if ((!xmlStrcmp(val, (const xmlChar *) "writer"))) type = SC_XMLTV_CREDIT_TYPE_WRITER;
        xmlFree(val);

        if (!xmlTextReaderIsEmptyElement(reader) &&
            xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT &&
            type > 0 &&
            xmlTextReaderDepth(reader) == SC_XMLTV_CREDIT_DEPTH + 1) {
            cred = (sc_xmltv_credit_t *) sc_xmltv_create(SC_XMLTV_CREDIT);
            cred->type = type;
            sc_xmltv_get_reader_element_value(reader, &cred->name);
            node = sc_list_node_create(cred);
            sc_list_node_append(*list, node);
        }
    }

//    node = NULL;
}

sc_xmltv_programme_t *sc_xmltv_parse_programme(xmlTextReaderPtr reader) {
    sc_xmltv_programme_t *prog = NULL;
    char *val = NULL;
    int ret;

    prog = (sc_xmltv_programme_t *) sc_xmltv_create(SC_XMLTV_PROGRAMME);

    sc_xmltv_get_reader_property_value(reader, "start", &val);
    prog->start = sc_xmltv_to_unix_time(val);
    free(val);
    val = NULL;

    sc_xmltv_get_reader_property_value(reader, "stop", &val);
    prog->stop = sc_xmltv_to_unix_time(val);
    free(val);
    val = NULL;

    sc_xmltv_get_reader_property_value(reader, "channel", &prog->channel);

    for (ret = xmlTextReaderRead(reader); ret == 1; ret = xmlTextReaderRead(reader)) {
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_END_ELEMENT, SC_XMLTV_PROGRAMME_NAME,
                                               SC_XMLTV_PROGRAMME_DEPTH)) {
            break;
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "title",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_element_value(reader, &prog->title);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "sub-title",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_element_value(reader, &prog->sub_title);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "desc", SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_element_value(reader, &prog->desc);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "credits",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_parse_credits(reader, &prog->credits);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "date", SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_element_value(reader, &prog->date);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "category",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_list_node_t *node = sc_list_node_create(NULL);
            sc_xmltv_get_reader_element_value(reader, (char **) (&node->data));
            sc_list_node_append(prog->categories, node);
//            node = NULL;
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "episode-num",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_property_value(reader, "system", &val);
            if (val && (!strcmp(val, "onscreen"))) {
                free(val);
                val = NULL;
                sc_xmltv_get_reader_element_value(reader, &val);
                uintmax_t num = strtoumax((const char *) val, NULL, 10);
                if (errno != ERANGE) {
                    prog->episode_num = (int) num;
                }
            }
            free(val);
            val = NULL;
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "previously-shown",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_property_value(reader, "start", &val);
            prog->start = sc_xmltv_to_unix_time(val);
            free(val);
            val = NULL;
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "star-rating",
                                               SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            int iret;
            for (iret = xmlTextReaderRead(reader); iret == 1; iret = xmlTextReaderRead(reader)) {
                if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_END_ELEMENT, "star-rating",
                                                       SC_XMLTV_PROGRAMME_DEPTH + 1)) {
                    break;
                }

                xmlChar *ival = xmlTextReaderName(reader);
                if ((!xmlStrcmp(ival, (const xmlChar *) "value"))) {
                    sc_xmltv_get_reader_element_value(reader, &prog->star_rating);
                }
                xmlFree(ival);
            }
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "icon", SC_XMLTV_PROGRAMME_DEPTH + 1)) {
            sc_xmltv_get_reader_property_value(reader, "src", &prog->icon);
        }
    }

    return prog;
}

sc_xmltv_channel_t *sc_xmltv_parse_channel(xmlTextReaderPtr reader) {
    sc_xmltv_channel_t *chan = NULL;
    int ret;

    chan = (sc_xmltv_channel_t *) sc_xmltv_create(SC_XMLTV_CHANNEL);
    sc_xmltv_get_reader_property_value(reader, "id", &chan->id_);

    for (ret = xmlTextReaderRead(reader); ret == 1; ret = xmlTextReaderRead(reader)) {
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_END_ELEMENT, SC_XMLTV_CHANNEL_NAME,
                                               SC_XMLTV_CHANNEL_DEPTH)) {
            break;
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, "display-name",
                                               SC_XMLTV_CHANNEL_DEPTH + 1)) {
            sc_list_node_t *node = sc_list_node_create(NULL);
            sc_xmltv_get_reader_element_value(reader, (char **) (&node->data));
            sc_list_node_append(chan->display_names, node);
//            node = NULL;
        }
    }

    return chan;
}

void sc_xmltv_link_progs_to_chan(sc_list_t *programmes, sc_xmltv_channel_t *chan) {
    sc_list_node_t *node = programmes->first;
    sc_list_node_t *ulnode = NULL;
    while (node) {
        sc_xmltv_programme_t *prog = (sc_xmltv_programme_t *) node->data;
        if ((!strcmp(prog->channel, chan->id_))) {
            ulnode = node;
            node = sc_list_node_unlink(programmes, ulnode);
            sc_list_node_append(chan->programmes, ulnode);
        } else {
            node = node->next;
        }
    }
}

sc_list_t *sc_xmltv_parse(const char *filename) {
    xmlTextReaderPtr reader = NULL;
    sc_list_t *channels = NULL;
    sc_list_t *programmes = NULL;
    int ret;
    sc_list_node_t *node = NULL;

    reader = xmlNewTextReaderFilename(filename);
    if (!reader)
        return NULL;

    channels = sc_list_create();
    programmes = sc_list_create();

    for (ret = xmlTextReaderRead(reader); ret == 1; ret = xmlTextReaderRead(reader)) {
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, SC_XMLTV_CHANNEL_NAME,
                                               SC_XMLTV_CHANNEL_DEPTH)) {
            node = sc_list_node_create(sc_xmltv_parse_channel(reader));
            sc_list_node_append(channels, node);
        }
        if (sc_xmltv_check_current_reader_node(reader, XML_READER_TYPE_ELEMENT, SC_XMLTV_PROGRAMME_NAME,
                                               SC_XMLTV_PROGRAMME_DEPTH)) {
            node = sc_list_node_create(sc_xmltv_parse_programme(reader));
            sc_list_node_append(programmes, node);
        }
    }

    xmlFreeTextReader(reader);

    node = channels->first;
    while (node) {
        sc_xmltv_link_progs_to_chan(programmes, (sc_xmltv_channel_t *) node->data);
        node = node->next;
    }

    sc_xmltv_list_free(SC_XMLTV_PROGRAMME, &programmes);

    return channels;
}
