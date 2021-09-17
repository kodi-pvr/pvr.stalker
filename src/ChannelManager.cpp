/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ChannelManager.h"

#include "Utils.h"

#include <cmath>
#include <kodi/General.h>

using namespace SC;

SError ChannelManager::LoadChannels()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  Json::Value parsed;
  int genre = 10;
  unsigned int currentPage = 1;
  unsigned int maxPages = 1;

  if (!m_api->ITVGetAllChannels(parsed) || !ParseChannels(parsed))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: ITVGetAllChannels failed", __func__);
    return SERROR_LOAD_CHANNELS;
  }

  while (currentPage <= maxPages)
  {
    kodi::Log(ADDON_LOG_DEBUG, "%s: currentPage: %d", __func__, currentPage);

    if (!m_api->ITVGetOrderedList(genre, currentPage, parsed) || !ParseChannels(parsed))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: ITVGetOrderedList failed", __func__);
      return SERROR_LOAD_CHANNELS;
    }

    if (currentPage == 1)
    {
      int totalItems = Utils::GetIntFromJsonValue(parsed["js"]["total_items"]);
      int maxPageItems = Utils::GetIntFromJsonValue(parsed["js"]["max_page_items"]);

      if (totalItems > 0 && maxPageItems > 0)
        maxPages = static_cast<unsigned int>(ceil((double)totalItems / maxPageItems));

      kodi::Log(ADDON_LOG_DEBUG, "%s: totalItems: %d | maxPageItems: %d | maxPages: %d", __func__,
                totalItems, maxPageItems, maxPages);
    }

    currentPage++;
  }

  return SERROR_OK;
}

bool ChannelManager::ParseChannels(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  if (!parsed.isMember("js") || !parsed["js"].isMember("data"))
    return false;

  Json::Value value;

  try
  {
    value = parsed["js"]["data"];
    if (!value.isObject() && !value.isArray())
      return false;

    for (Json::Value::iterator it = value.begin(); it != value.end(); ++it)
    {
      Channel channel;
      channel.uniqueId = GetChannelId((*it)["name"].asCString(), (*it)["number"].asCString());
      channel.number = std::stoi((*it)["number"].asString());
      channel.name = (*it)["name"].asString();

      channel.streamUrl = "pvr://stream/" + std::to_string(channel.uniqueId);

      std::string strLogo = (*it)["logo"].asString();
      channel.iconPath = Utils::DetermineLogoURI(m_api->GetBasePath(), strLogo);

      channel.channelId = Utils::GetIntFromJsonValue((*it)["id"]);
      channel.cmd = (*it)["cmd"].asString();
      channel.tvGenreId = (*it)["tv_genre_id"].asString();
      channel.useHttpTmpLink = !!Utils::GetIntFromJsonValue((*it)["use_http_tmp_link"]);
      channel.useLoadBalancing = !!Utils::GetIntFromJsonValue((*it)["use_load_balancing"]);

      m_channels.push_back(channel);

      kodi::Log(ADDON_LOG_DEBUG, "%s: %d - %s", __func__, channel.number, channel.name.c_str());
    }
  }
  catch (const std::exception& ex)
  {
    return false;
  }

  return true;
}

//https://github.com/kodi-pvr/pvr.iptvsimple/blob/master/src/PVRIptvData.cpp#L1085
int ChannelManager::GetChannelId(const char* strChannelName, const char* strStreamUrl)
{
  std::string concat(strChannelName);
  concat.append(strStreamUrl);

  const char* strString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *strString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}

SError ChannelManager::LoadChannelGroups()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  Json::Value parsed;

  // genres are channel groups
  if (!m_api->ITVGetGenres(parsed) || !ParseChannelGroups(parsed))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: ITVGetGenres|ParseChannelGroups failed", __func__);
    return SERROR_LOAD_CHANNEL_GROUPS;
  }

  return SERROR_OK;
}

bool ChannelManager::ParseChannelGroups(Json::Value& parsed)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  if (!parsed.isMember("js"))
    return false;

  Json::Value value;

  try
  {
    value = parsed["js"];
    if (!value.isObject() && !value.isArray())
      return false;

    for (Json::Value::iterator it = value.begin(); it != value.end(); ++it)
    {
      ChannelGroup channelGroup;
      channelGroup.name = (*it)["title"].asString();
      if (!channelGroup.name.empty())
        channelGroup.name[0] = (char)toupper(channelGroup.name[0]);
      channelGroup.id = (*it)["id"].asString();
      channelGroup.alias = (*it)["alias"].asString();

      m_channelGroups.push_back(channelGroup);

      kodi::Log(ADDON_LOG_DEBUG, "%s: %s - %s", __func__, channelGroup.id.c_str(),
                channelGroup.name.c_str());
    }
  }
  catch (const std::exception& ex)
  {
    return false;
  }

  return true;
}

std::string ChannelManager::GetStreamURL(Channel& channel)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::string cmd;
  Json::Value parsed;
  size_t pos;

  // /c/player.js#L2198
  if (channel.useHttpTmpLink || channel.useLoadBalancing)
  {
    kodi::Log(ADDON_LOG_DEBUG, "%s: getting temp stream url", __func__);

    if (!m_api->ITVCreateLink(channel.cmd, parsed))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: ITVCreateLink failed", __func__);
      return cmd;
    }

    cmd = ParseStreamCmd(parsed);
  }
  else
  {
    cmd = channel.cmd;
  }

  // cmd format
  // (?:ffrt\d*\s|)(.*)
  if ((pos = cmd.find(" ")) != std::string::npos)
    cmd = cmd.substr(pos + 1);

  return cmd;
}

std::string ChannelManager::ParseStreamCmd(Json::Value& parsed)
{
  std::string cmd;

  if (parsed.isMember("js") && parsed["js"].isMember("cmd"))
    cmd = parsed["js"]["cmd"].asString();

  return cmd;
}
