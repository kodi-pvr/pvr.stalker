/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string>

namespace Base
{
class Cache
{
public:
  Cache() = default;
  virtual ~Cache() = default;

protected:
  virtual bool Open(const std::string& cacheFile,
                    xmlDocPtr& doc,
                    xmlNodePtr& rootNode,
                    const std::string& rootNodeName);

  virtual xmlNodePtr FindNodeByName(xmlNodePtr& startNode, const xmlChar* name);

  virtual xmlNodePtr FindAndGetNodeValue(xmlNodePtr& parentNode,
                                         const xmlChar* name,
                                         std::string& value);

  virtual xmlNodePtr FindAndSetNodeValue(xmlNodePtr& parentNode,
                                         const xmlChar* name,
                                         const xmlChar* value);
};
} // namespace Base
