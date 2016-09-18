#pragma once

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

#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>

namespace Base {
    class Cache {
    public:
        Cache();

        virtual ~Cache();

    protected:
        virtual bool Open(const std::string &cacheFile, xmlDocPtr &doc, xmlNodePtr &rootNode,
                          const std::string &rootNodeName);

        virtual xmlNodePtr FindNodeByName(xmlNodePtr &startNode, const xmlChar *name);

        virtual xmlNodePtr FindAndGetNodeValue(xmlNodePtr &parentNode, const xmlChar *name, std::string &value);

        virtual xmlNodePtr FindAndSetNodeValue(xmlNodePtr &parentNode, const xmlChar *name, const xmlChar *value);
    };
}
