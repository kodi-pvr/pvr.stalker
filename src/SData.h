#pragma once
/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include <jsoncpp/include/json/json.h>
#include <platform/util/StdString.h>

#include "client.h"

struct SEpgEntry
{
  int         iBroadcastId;
  std::string strTitle;
  int         iChannelId;
  time_t      startTime;
  time_t      endTime;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strIconPath;
  int         iGenreType;
  int         iGenreSubType;
//  time_t      firstAired;
//  int         iParentalRating;
//  int         iStarRating;
//  bool        bNotify;
//  int         iSeriesNumber;
//  int         iEpisodeNumber;
//  int         iEpisodePartNumber;
//  std::string strEpisodeName;
};

struct SChannel
{
  bool                    bRadio;
  int                     iUniqueId;
  int                     iChannelNumber;
  int                     iSubChannelNumber;
  int                     iEncryptionSystem;
  std::string             strChannelName;
  std::string             strIconPath;
  std::string             strStreamURL;
  int                     iChannelId;
  std::string             cmd;
  bool                    use_http_tmp_link;
  bool                    use_load_balancing;
  std::vector<SEpgEntry> epg;
};

struct SRecording
{
  int         iDuration;
  int         iGenreType;
  int         iGenreSubType;
  std::string strChannelName;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strRecordingId;
  std::string strStreamURL;
  std::string strTitle;
  std::string strDirectory;
  time_t      recordingTime;
};

struct STimer
{
  int             iChannelId;
  time_t          startTime;
  time_t          endTime;
  PVR_TIMER_STATE state;
  std::string     strTitle;
  std::string     strSummary;
};

struct SChannelGroup
{
  bool             bRadio;
  int              iGroupId;
  std::string      strGroupName;
  std::vector<int> members;
};

class SData
{
public:
  SData(void);
  virtual ~SData(void);
  
  virtual bool LoadData(void);

  virtual int GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  virtual bool GetChannel(const PVR_CHANNEL &channel, SChannel &myChannel);

  virtual int GetChannelGroupsAmount(void);
  virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  virtual int GetRecordingsAmount(void);
  virtual PVR_ERROR GetRecordings(ADDON_HANDLE handle);

  virtual int GetTimersAmount(void);
  virtual PVR_ERROR GetTimers(ADDON_HANDLE handle);

  virtual std::string GetSettingsFile(std::string settingFile) const;
  virtual const char* GetChannelStreamURL(const PVR_CHANNEL &channel);
protected:
	virtual bool LoadCache();
	virtual bool SaveCache();
  virtual bool InitAPI();
  virtual bool LoadProfile();
	virtual bool Authenticate();
	virtual bool ParseChannels(Json::Value &parsed);
	virtual bool LoadChannels();
  virtual bool ParseEPG(Json::Value &parsed, time_t iStart, time_t iEnd, int iChannelNumber, ADDON_HANDLE handle);
  virtual bool LoadEPGForChannel(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle);
  virtual bool LoadEPGForChannel2(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle);
  virtual std::string GetEPGCachePath();
  virtual bool DownloadEPG(time_t iStart, time_t iEnd, SChannel &channel);
  virtual bool LoadEPGFromFile(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle);
	virtual bool LoadEPGForChannel3(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle);

  virtual int GetChannelId(const char * strChannelName, const char * strNumber);
	virtual int GetIntValue(Json::Value &value);
	virtual bool GetIntValueAsBool(Json::Value &value);

  virtual std::vector<std::string> split(std::string str, char delimiter);
private:
  std::vector<SChannelGroup> m_groups;
  std::vector<SChannel>      m_channels;
  std::vector<SRecording>    m_recordings;
  std::vector<STimer>        m_timers;
  time_t                           m_iEpgStart;
  CStdString                       m_strDefaultIcon;
  CStdString                       m_strDefaultMovie;

  bool                        m_bApiInit;
  bool                        m_bAuthenticated;
  bool                        m_bProfileLoaded;
  bool                        m_epgDownloaded;
	Json::Value                 m_epgWeek;
	Json::Value                 m_epgData;
  std::string                 m_PlaybackURL;
};
