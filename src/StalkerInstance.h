/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "stalker/CWatchdog.h"
#include "stalker/ChannelManager.h"
#include "stalker/Error.h"
#include "stalker/GuideManager.h"
#include "stalker/SAPI.h"
#include "stalker/SessionManager.h"
#include "stalker/InstanceSettings.h"
#include "stalker/XMLTV.h"
#include "stalker/base/Cache.h"
#include "libstalkerclient/identity.h"
#include "libstalkerclient/stb.h"

#include <json/json.h>
#include <kodi/addon-instance/PVR.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Stalker
{

class ATTR_DLL_LOCAL StalkerInstance : public kodi::addon::CInstancePVRClient,
                                       private Base::Cache
{
public:
  StalkerInstance(const kodi::addon::IInstanceInfo& instance);
  ~StalkerInstance();

  ADDON_STATUS SetInstanceSetting(const std::string& settingName,
                                  const kodi::addon::CSettingValue& settingValue) override
  {
    return ADDON_STATUS_NEED_RESTART;
  }

  ADDON_STATUS Initialize();

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR GetConnectionString(std::string& connection) override;

  PVR_ERROR GetEPGForChannel(int channelUid,
                             time_t start,
                             time_t end,
                             kodi::addon::PVREPGTagsResultSet& results) override;

  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                   kodi::addon::PVRChannelGroupMembersResultSet& results) override;

  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  PVR_ERROR GetChannelStreamProperties(
      const kodi::addon::PVRChannel& channel,
      std::vector<kodi::addon::PVRStreamProperty>& properties) override;

  std::shared_ptr<Stalker::InstanceSettings> settings;

protected:
  bool LoadCache();

  bool SaveCache();

  bool IsAuthenticated() const;

  SError Authenticate();

  void QueueErrorNotification(SError error) const;

private:
  bool ConfigureStalkerAPISettings();
  std::string GetChannelStreamURL(const kodi::addon::PVRChannel& channel) const;

  bool m_tokenManuallySet = false;
  time_t m_lastEpgAccessTime = 0;
  time_t m_nextEpgLoadTime = 0;
  sc_identity_t m_identity;
  sc_stb_profile_t m_profile;
  bool m_epgThreadActive = false;
  std::thread m_epgThread;
  mutable std::mutex m_epgMutex;
  Stalker::SAPI* m_api = new Stalker::SAPI;
  Stalker::SessionManager* m_sessionManager = new Stalker::SessionManager;
  Stalker::ChannelManager* m_channelManager = new Stalker::ChannelManager;
  Stalker::GuideManager* m_guideManager = new Stalker::GuideManager;
};

} // SC