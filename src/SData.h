/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "CWatchdog.h"
#include "ChannelManager.h"
#include "Error.h"
#include "GuideManager.h"
#include "SAPI.h"
#include "SessionManager.h"
#include "Settings.h"
#include "XMLTV.h"
#include "base/Cache.h"
#include "client.h"
#include "libstalkerclient/identity.h"
#include "libstalkerclient/stb.h"

#include <json/json.h>
#include <mutex>
#include <thread>
#include <vector>

class SData : Base::Cache
{
public:
  SData();

  ~SData();

  bool ReloadSettings();

  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t start, time_t anEnd);

  int GetChannelGroupsAmount();

  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool radio);

  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);

  int GetChannelsAmount();

  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool radio);

  PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel,
                                       PVR_NAMED_VALUE* properties,
                                       unsigned int* iPropertiesCount);

  SC::Settings settings;

protected:
  bool LoadCache();

  bool SaveCache();

  bool IsAuthenticated() const;

  SError Authenticate();

  void QueueErrorNotification(SError error) const;

private:
  std::string GetChannelStreamURL(const PVR_CHANNEL& channel) const;

  bool m_tokenManuallySet = false;
  time_t m_lastEpgAccessTime = 0;
  time_t m_nextEpgLoadTime = 0;
  sc_identity_t m_identity;
  sc_stb_profile_t m_profile;
  bool m_epgThreadActive = false;
  std::thread m_epgThread;
  mutable std::mutex m_epgMutex;
  SC::SAPI* m_api = new SC::SAPI;
  SC::SessionManager* m_sessionManager = new SC::SessionManager;
  SC::ChannelManager* m_channelManager = new SC::ChannelManager;
  SC::GuideManager* m_guideManager = new SC::GuideManager;
};
