/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "GuideManager.h"

#include "Utils.h"

#include <chrono>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <thread>

using namespace SC;

GuideManager::~GuideManager()
{
  m_api = nullptr;
  Clear();
}

SError GuideManager::LoadGuide(time_t start, time_t end)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_XMLTV_ONLY)
    return SERROR_OK;

  bool ret(false);
  int maxRetires(5);
  int numRetries(0);
  int period;
  std::string cacheFile;
  unsigned int cacheExpiry(0);

  //TODO limit period to 24 hours for ITVGetEPGInfo. large amount of channels over too many days could exceed server max memory allowed for that request
  period = (int)(end - start) / 3600;

  if (m_useCache)
  {
    cacheFile = Utils::GetFilePath("epg_provider.json");
    cacheExpiry = m_expiry;
  }

  while (!ret && ++numRetries <= maxRetires)
  {
    // don't sleep on first try
    if (numRetries > 1)
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    if (!(ret = m_api->ITVGetEPGInfo(period, m_epgData, cacheFile, cacheExpiry)))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: ITVGetEPGInfo failed", __func__);
      if (m_useCache && kodi::vfs::FileExists(cacheFile, false))
      {
        kodi::vfs::DeleteFile(cacheFile);
      }
    }
  }

  return ret ? SERROR_OK : SERROR_LOAD_EPG;
}

SError GuideManager::LoadXMLTV(HTTPSocket::Scope scope, const std::string& path)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PROVIDER_ONLY || path.empty())
    return SERROR_OK;

  bool ret(false);
  int maxRetires(5);
  int numRetries(0);

  m_xmltv->SetUseCache(m_useCache);
  m_xmltv->SetCacheFile(Utils::GetFilePath("epg_xmltv.xml"));
  m_xmltv->SetCacheExpiry(m_expiry);

  while (!ret && ++numRetries <= maxRetires)
  {
    // don't sleep on first try
    if (numRetries > 1)
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    if (!(ret = m_xmltv->Parse(scope, path)))
      kodi::Log(ADDON_LOG_ERROR, "%s: XMLTV Parse failed", __func__);
  }

  return ret ? SERROR_OK : SERROR_LOAD_EPG;
}

int GuideManager::AddEvents(
    int type, std::vector<Event>& events, Channel& channel, time_t start, time_t end)
{
  int addedEvents(0);

  if (type == 0)
  {
    try
    {
      std::string channelId;

      channelId = std::to_string(channel.channelId);
      if (!m_epgData.isMember("js") || !m_epgData["js"].isObject() ||
          !m_epgData["js"].isMember("data") || !m_epgData["js"]["data"].isMember(channelId))
      {
        return addedEvents;
      }

      Json::Value value;
      time_t startTimestamp;
      time_t stopTimestamp;

      value = m_epgData["js"]["data"][channelId];
      if (!value.isObject() && !value.isArray())
        return addedEvents;

      for (Json::Value::iterator it = value.begin(); it != value.end(); ++it)
      {
        try
        {
          startTimestamp = Utils::GetIntFromJsonValue((*it)["start_timestamp"]);
          stopTimestamp = Utils::GetIntFromJsonValue((*it)["stop_timestamp"]);

          if (start != 0 && end != 0 && !(startTimestamp >= start && stopTimestamp <= end))
            continue;

          Event e;
          e.uniqueBroadcastId = Utils::GetIntFromJsonValue((*it)["id"]);
          e.title = (*it)["name"].asCString();
          e.channelNumber = channel.number;
          e.startTime = startTimestamp;
          e.endTime = stopTimestamp;
          e.plot = (*it)["descr"].asCString();

          events.push_back(e);
          addedEvents++;
        }
        catch (const std::exception& ex)
        {
          kodi::Log(ADDON_LOG_DEBUG, "%s: epg event excep. what=%s", __func__, ex.what());
        }
      }
    }
    catch (const std::exception& ex)
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: epg data excep. what=%s", __func__, ex.what());
    }
  }

  if (type == 1)
  {
    std::string channelNum;
    XMLTV::Channel* c;

    channelNum = std::to_string(channel.number);
    c = m_xmltv->GetChannelById(channelNum);
    if (c == nullptr)
    {
      c = m_xmltv->GetChannelByDisplayName(channel.name);
    }
    if (c != nullptr)
    {
      for (std::vector<XMLTV::Programme>::iterator it = c->programmes.begin();
           it != c->programmes.end(); ++it)
      {
        XMLTV::Programme* p = &(*it);

        if (start != 0 && end != 0 && !(p->start >= start && p->stop <= end))
          continue;

        Event e;
        e.uniqueBroadcastId = p->extra.broadcastId;
        e.title = p->title;
        e.channelNumber = channel.number;
        e.startTime = p->start;
        e.endTime = p->stop;
        e.plot = p->desc;
        e.cast = p->extra.cast;
        e.directors = p->extra.directors;
        e.writers = p->extra.writers;
        if (!p->date.empty())
          e.year = std::stoi(p->date.substr(0, 4));
        e.iconPath = p->icon;
        e.genreType = p->extra.genreType;
        e.genreDescription = p->extra.genreDescription;
        e.firstAired = p->previouslyShown;
        if (!p->starRating.empty())
          e.starRating = std::stoi(p->starRating.substr(0, 1));
        e.episodeNumber = p->episodeNumber;
        e.episodeName = p->subTitle;

        events.push_back(e);
        addedEvents++;
      }
    }
  }

  return addedEvents;
}

std::vector<Event> GuideManager::GetChannelEvents(Channel& channel, time_t start, time_t end)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::vector<Event> events;
  int addedEvents;

  if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_PROVIDER ||
      m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PROVIDER_ONLY)
  {
    addedEvents = AddEvents(0, events, channel, start, end);
    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_PROVIDER && !addedEvents)
      AddEvents(1, events, channel, start, end);
  }

  if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_XMLTV ||
      m_guidePreference == SC::Settings::GUIDE_PREFERENCE_XMLTV_ONLY)
  {
    addedEvents = AddEvents(1, events, channel, start, end);
    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_XMLTV && !addedEvents)
      AddEvents(0, events, channel, start, end);
  }

  return events;
}

void GuideManager::Clear()
{
  m_epgData.clear();
  m_xmltv->Clear();
}
