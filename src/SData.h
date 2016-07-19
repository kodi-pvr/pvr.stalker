#pragma once

/*
 *      Copyright (C) 2015, 2016  Jamal Edey
 *      http://www.kenshisoft.com/
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

#include <vector>

#include <json/json.h>

#include "libstalkerclient/identity.h"
#include "libstalkerclient/stb.h"
#include "base/Cache.h"
#include "client.h"
#include "ChannelManager.h"
#include "CWatchdog.h"
#include "Error.h"
#include "GuideManager.h"
#include "SAPI.h"
#include "Settings.h"
#include "XMLTV.h"

class SData : Base::Cache
{
public:
  SData(void);
  virtual ~SData(void);
  
  virtual bool LoadData(void);
  virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  virtual int GetChannelGroupsAmount(void);
  virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  virtual int GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  virtual const char* GetChannelStreamURL(const PVR_CHANNEL &channel);

  virtual SError ReAuthenticate(bool bAuthorizationLost = false);
  virtual void UnloadEPG();

  SC::Settings settings;
protected:
  virtual bool LoadCache();
  virtual bool SaveCache();
  virtual SError InitAPI();
  virtual SError DoHandshake();
  virtual SError DoAuth();
  virtual SError LoadProfile(bool bAuthSecondStep = false);
  virtual SError Authenticate();
  virtual bool IsInitialized();
  virtual SError Initialize();

  virtual void QueueErrorNotification(SError error);
private:
  std::string                 m_strLastUnknownError;
  bool                        m_bInitedApi;
  bool                        m_bTokenManuallySet;
  bool                        m_bAuthenticated;
  time_t                      m_iLastEpgAccessTime;
  time_t                      m_iNextEpgLoadTime;
  
  sc_identity_t               m_identity;
  P8PLATFORM::CMutex            m_authMutex;
  sc_stb_profile_t            m_profile;
  std::string                 m_PlaybackURL;
  CWatchdog                   *m_watchdog;
  P8PLATFORM::CMutex            m_epgMutex;
  SC::SAPI                      *m_api;
  SC::ChannelManager            *m_channelManager;
  SC::GuideManager              *m_guideManager;
};
