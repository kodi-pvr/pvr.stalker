#pragma once

/*
 *      Copyright (C) 2015  Jamal Edey
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
#include "client.h"
#include "CWatchdog.h"
#include "XMLTV.h"

typedef enum {
  SERROR_OK = 1,
  SERROR_UNKNOWN = -1,
  SERROR_INITIALIZE = -2,
  SERROR_API = -3,
  SERROR_AUTHENTICATION = -4,
  SERROR_LOAD_CHANNELS = -5,
  SERROR_LOAD_CHANNEL_GROUPS = -6,
  SERROR_STREAM_URL = -7,
  SERROR_AUTHORIZATION = -8
} SError;

struct SChannelGroup
{
  std::string strGroupName;
  bool        bRadio;
  std::string strId;
  std::string strAlias;
};

struct SChannel
{
  int         iUniqueId;
  bool        bRadio;
  int         iChannelNumber;
  std::string strChannelName;
  std::string strStreamURL;
  std::string strIconPath;
  int         iChannelId;
  std::string strCmd;
  std::string strTvGenreId;
  bool        bUseHttpTmpLink;
  bool        bUseLoadBalancing;
};

class SData
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
  virtual int ParseEPG(Json::Value &parsed, time_t iStart, time_t iEnd, int iChannelNumber, ADDON_HANDLE handle);
  virtual int ParseEPGXMLTV(int iChannelNumber, std::string &strChannelName, time_t iStart, time_t iEnd, ADDON_HANDLE handle);
  virtual SError LoadEPGForChannel(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle);
  virtual bool ParseChannelGroups(Json::Value &parsed);
  virtual SError LoadChannelGroups();
  virtual bool ParseChannels(Json::Value &parsed);
  virtual SError LoadChannels();

  virtual void QueueErrorNotification(SError error);
  virtual std::string GetFilePath(std::string strPath, bool bUserPath = true);
  virtual int GetChannelId(const char * strChannelName, const char * strNumber);
private:
  std::string                 m_strLastUnknownError;
  bool                        m_bInitedApi;
  bool                        m_bTokenManuallySet;
  bool                        m_bAuthenticated;
  uint64_t                    m_iNextEpgLoadTime;
  
  sc_identity_t               m_identity;
  PLATFORM::CMutex            m_authMutex;
  sc_stb_profile_t            m_profile;
  Json::Value                 m_epgData;
  std::vector<SChannelGroup>  m_channelGroups;
  std::vector<SChannel>       m_channels;
  std::string                 m_PlaybackURL;
  CWatchdog                   *m_watchdog;
  XMLTV                       *m_xmltv;
};
