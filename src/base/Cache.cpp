/*
 *      Copyright (C) 2016  Jamal Edey
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

#include "Cache.h"

using namespace Base;

Cache::Cache() {
}

Cache::~Cache() {
}

bool Cache::Open(const std::string &cacheFile, xmlDocPtr &doc, xmlNodePtr &rootNode, const std::string &rootNodeName) {
    bool failed(false);
    doc = xmlReadFile(cacheFile.c_str(), NULL, 0);
    if (!doc) {
//        fprintf(stderr, "%s: failed to read cache file", __FUNCTION__);
        failed = true;
    } else {
        rootNode = xmlDocGetRootElement(doc);
        if (!rootNode || xmlStrcmp(rootNode->name, (const xmlChar *) rootNodeName.c_str())) {
//            fprintf(stderr, "%s: 'cache' root element not found", __FUNCTION__);
            failed = true;
        }
    }
    return !failed;
}

xmlNodePtr Cache::FindNodeByName(xmlNodePtr &startNode, const xmlChar *name) {
    xmlNodePtr node = NULL;
    for (node = startNode; node; node = node->next) {
//        fprintf(stderr, "%s: name=%s\n", __FUNCTION__, node->name);
        if (!xmlStrcmp(node->name, name)) {
            return node;
        }
    }
    return NULL;
}

xmlNodePtr Cache::FindAndGetNodeValue(xmlNodePtr &parentNode, const xmlChar *name, std::string &value) {
    xmlNodePtr node = NULL;
    node = FindNodeByName(parentNode->children, name);
    if (!node) {
//        fprintf(stderr, "%s: '%s' element not found\n", __FUNCTION__, name);
    } else {
        xmlChar *val = NULL;
        val = xmlNodeGetContent(node);
        if (val) {
            value = (const char *) val;
        }
        xmlFree(val);
    }
    return node;
}

xmlNodePtr Cache::FindAndSetNodeValue(xmlNodePtr &parentNode, const xmlChar *name, const xmlChar *value) {
    xmlNodePtr node = NULL;
    node = FindNodeByName(parentNode->children, name);
    if (!node) {
        node = xmlNewChild(parentNode, NULL, name, NULL);
    }
    xmlNodeSetContent(node, value);
    return node;
}
