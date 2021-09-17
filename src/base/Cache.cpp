/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Cache.h"

using namespace Base;

bool Cache::Open(const std::string& cacheFile,
                 xmlDocPtr& doc,
                 xmlNodePtr& rootNode,
                 const std::string& rootNodeName)
{
  bool failed(false);
  doc = xmlReadFile(cacheFile.c_str(), nullptr, 0);
  if (!doc)
  {
    //        fprintf(stderr, "%s: failed to read cache file", __func__);
    failed = true;
  }
  else
  {
    rootNode = xmlDocGetRootElement(doc);
    if (!rootNode || xmlStrcmp(rootNode->name, (const xmlChar*)rootNodeName.c_str()))
    {
      //            fprintf(stderr, "%s: 'cache' root element not found", __func__);
      failed = true;
    }
  }
  return !failed;
}

xmlNodePtr Cache::FindNodeByName(xmlNodePtr& startNode, const xmlChar* name)
{
  xmlNodePtr node = nullptr;
  for (node = startNode; node; node = node->next)
  {
    //        fprintf(stderr, "%s: name=%s\n", __func__, node->name);
    if (!xmlStrcmp(node->name, name))
    {
      return node;
    }
  }
  return nullptr;
}

xmlNodePtr Cache::FindAndGetNodeValue(xmlNodePtr& parentNode,
                                      const xmlChar* name,
                                      std::string& value)
{
  xmlNodePtr node = nullptr;
  node = FindNodeByName(parentNode->children, name);
  if (!node)
  {
    //        fprintf(stderr, "%s: '%s' element not found\n", __func__, name);
  }
  else
  {
    xmlChar* val = nullptr;
    val = xmlNodeGetContent(node);
    if (val)
    {
      value = (const char*)val;
    }
    xmlFree(val);
  }
  return node;
}

xmlNodePtr Cache::FindAndSetNodeValue(xmlNodePtr& parentNode,
                                      const xmlChar* name,
                                      const xmlChar* value)
{
  xmlNodePtr node = nullptr;
  node = FindNodeByName(parentNode->children, name);
  if (!node)
  {
    node = xmlNewChild(parentNode, nullptr, name, nullptr);
  }
  xmlNodeSetContent(node, value);
  return node;
}
