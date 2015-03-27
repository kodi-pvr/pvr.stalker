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

#include "SData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tinyxml/tinyxml.h>

#include "SAPI.h"

using namespace ADDON;

SData::SData(void)
{
  m_iEpgStart = -1;
  m_strDefaultIcon =  "http://www.royalty-free.tv/news/wp-content/uploads/2011/06/cc-logo1.jpg";
  m_strDefaultMovie = "";

  LoadDemoData();
}

SData::~SData(void)
{
	XBMC->Log(LOG_ERROR, "->~SData()");

  m_channels.clear();
  m_groups.clear();
}

std::string SData::GetSettingsFile(std::string settingFile) const
{
  //string settingFile = g_strClientPath;
  if (settingFile.at(settingFile.size() - 1) == '\\' ||
      settingFile.at(settingFile.size() - 1) == '/')
    settingFile.append("cache.xml");
  else
    settingFile.append("/cache.xml");
  return settingFile;
}

bool SData::LoadCache()
{
	std::string strUSettingsFile;
	bool bUSettingsFileExists;
	std::string strSettingsFile;
	TiXmlDocument xml_doc;
	TiXmlElement *pRootElement = NULL;
	TiXmlElement *pTokenElement = NULL;

	strUSettingsFile = GetSettingsFile(g_strUserPath);
	bUSettingsFileExists = XBMC->FileExists(strUSettingsFile.c_str(), false);
	strSettingsFile = bUSettingsFileExists ? strUSettingsFile : GetSettingsFile(g_strClientPath);

	if (!xml_doc.LoadFile(strSettingsFile)) {
		XBMC->Log(LOG_ERROR, "failed to load: %s\n", strSettingsFile.c_str());
		return false;
	}

	if (!bUSettingsFileExists) {
		XBMC->Log(LOG_ERROR, "saving cache to user path\n");

		if (!xml_doc.SaveFile(strUSettingsFile)) {
			XBMC->Log(LOG_ERROR, "failed to save %s", strUSettingsFile.c_str());
			return false;
		}

		XBMC->Log(LOG_ERROR, "reloading cache from user path\n");
		return LoadCache();
	}

	pRootElement = xml_doc.RootElement();
	if (strcmp(pRootElement->Value(), "cache") != 0) {
		XBMC->Log(LOG_ERROR, "invalid xml doc. root tag 'cache' not found\n");
		return false;
	}
	
	pTokenElement = pRootElement->FirstChildElement("token");
	if (!pTokenElement) {
		XBMC->Log(LOG_ERROR, "token element not found\n");
	}
	else if (pTokenElement->GetText()) {
		g_token = pTokenElement->GetText();
		g_authorized = true;
	}

	XBMC->Log(LOG_ERROR, "token: %s\n", g_token.c_str());

	return true;
}

bool SData::SaveCache()
{
	std::string strSettingsFile;
	TiXmlDocument xml_doc;
	TiXmlElement *pRootElement = NULL;
	TiXmlElement *pTokenElement = NULL;

	strSettingsFile = GetSettingsFile(g_strUserPath);

	if (!xml_doc.LoadFile(strSettingsFile)) {
		XBMC->Log(LOG_ERROR, "failed to load: %s\n", strSettingsFile.c_str());
		return false;
	}

	pRootElement = xml_doc.RootElement();
	if (strcmp(pRootElement->Value(), "cache") != 0) {
		XBMC->Log(LOG_ERROR, "invalid xml doc. root tag 'cache' not found\n");
		return false;
	}

	pTokenElement = pRootElement->FirstChildElement("token");
	pTokenElement->Clear();
	pTokenElement->LinkEndChild(new TiXmlText(g_token));

	strSettingsFile = GetSettingsFile(g_strUserPath);
	if (!xml_doc.SaveFile(strSettingsFile)) {
		XBMC->Log(LOG_ERROR, "failed to save: %s", strSettingsFile.c_str());
		return false;
	}

	return true;
}

bool SData::Authenticate()
{
	Json::Value parsed;

	XBMC->Log(LOG_ERROR, "authenticating\n");

	if (!SAPI::Handshake(&parsed) || !SAPI::GetProfile(&parsed)) {
		XBMC->Log(LOG_ERROR, "%s: authentication failed\n", __FUNCTION__);
		return false;
	}

	XBMC->Log(LOG_ERROR, "saving token\n");

	if (!SaveCache()) {
		return false;
	}

	return true;
}

bool SData::ParseChannels(Json::Value &parsed)
{
	Json::Value tempValue;
	const char *tempChar = NULL;

	for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
		SChannel channel;
		channel.iUniqueId = ++g_iUniqueChannelId;

		/* channel name */
		channel.strChannelName = (*it)["name"].asString();

		/* radio/TV */
		channel.bRadio = false;

		/* channel number */
		tempChar = (*it)["number"].asCString();
		channel.iChannelNumber = atoi(tempChar);

		/* sub channel number */
		channel.iSubChannelNumber = 0;

		/* CAID */
		channel.iEncryptionSystem = 0;

		/* icon path */
		channel.strIconPath = "";

		channel.cmd = (*it)["cmd"].asString();

		tempValue = (*it)["use_http_tmp_link"];
		if (tempValue.isString()) {
			tempChar = tempValue.asCString();
			channel.use_http_tmp_link = atoi(tempChar) > 0 ? true : false;
		}
		else if (tempValue.isInt()) {
			channel.use_http_tmp_link = tempValue.asInt() > 0 ? true : false;
		}

		/* stream url */
		channel.strStreamURL = "pvr://stream/" + channel.cmd; // "pvr://stream/" causes GetLiveStreamURL to be called

		m_channels.push_back(channel);

		//XBMC->Log(LOG_ERROR, "%d - %s\n", channel.iChannelNumber, channel.strChannelName.c_str());
	}

	return true;
}

bool SData::LoadChannels()
{
	Json::Value parsed;
	g_iUniqueChannelId = 0;

	if (!SAPI::GetAllChannels(&parsed) || !ParseChannels(parsed)) {
		XBMC->Log(LOG_ERROR, "%s: get all channels failed\n", __FUNCTION__);
		return false;
	}

	parsed.clear();

	if (!SAPI::GetOrderedList(&parsed) || !ParseChannels(parsed)) {
		XBMC->Log(LOG_ERROR, "%s: get ordered list failed\n", __FUNCTION__);
		return false;
	}

	return true;
}

bool SData::LoadDemoData(void)
{
	LoadCache();
	
	if (!SAPI::Init()) {
		XBMC->Log(LOG_ERROR, "failed to init api\n");
		XBMC->QueueNotification(QUEUE_ERROR, "Startup failed.");
		return false;
	}

	if (g_token.empty() && !Authenticate()) {
		XBMC->QueueNotification(QUEUE_ERROR, "Authentication failed.");
		return false;
	}

	if (!LoadChannels()) {
		if (!g_authorized) {
			XBMC->Log(LOG_ERROR, "re-authenticating\n");
			if (!Authenticate()) {
				XBMC->QueueNotification(QUEUE_ERROR, "Authentication failed.");
				return false;
			}
		}
		
		XBMC->Log(LOG_ERROR, "re-attempting to load channels\n");
		if (!LoadChannels()) {
			XBMC->QueueNotification(QUEUE_ERROR, "Unable to load channels.");
			return false;
		}
	}

	return true;
}

int SData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR SData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    SChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      xbmcChannel.iSubChannelNumber = channel.iSubChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
      strncpy(xbmcChannel.strStreamURL, channel.strStreamURL.c_str(), sizeof(xbmcChannel.strStreamURL) - 1);
      xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

const char* SData::GetChannelStreamURL(const PVR_CHANNEL &channel)
{
	SChannel *thisChannel = NULL;

	for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++) {
		thisChannel = &m_channels.at(iChannelPtr);

		if (thisChannel->iUniqueId == (int)channel.iUniqueId) {
			break;
		}
	}

	if (!thisChannel) {
		XBMC->Log(LOG_ERROR, "channel not found\n");
		return "";
	}

	if (thisChannel->use_http_tmp_link) {
		XBMC->Log(LOG_ERROR, "getting temp stream url\n");

		Json::Value parsed;
		const char *cmd;
		const char *streamUrl;

		if (!SAPI::CreateLink(thisChannel->cmd, &parsed)) {
			XBMC->Log(LOG_ERROR, "%s: failed\n", __FUNCTION__);
			return "";
		}

		if (!parsed["js"].isMember("cmd")) {
			XBMC->Log(LOG_ERROR, "no stream url found\n");
			return "";
		}

		cmd = parsed["js"]["cmd"].asCString();

		// ffrt\d*\shttp://actual-stream-url.com/playlist.m3u8
		if ((streamUrl = strstr(cmd, " ")) == NULL) {
			XBMC->Log(LOG_ERROR, "no stream url found\n");
			return "";
		}

		m_PlaybackURL.assign(streamUrl + 1);
	}
	else {
		size_t pos;

		// ffrt\d*\shttp://actual-stream-url.com/playlist.m3u8
		if ((pos = thisChannel->cmd.find(" ")) == std::string::npos) {
			XBMC->Log(LOG_ERROR, "no stream url found\n");
			return "";
		}

		m_PlaybackURL = thisChannel->cmd.substr(pos + 1);
	}

	//XBMC->Log(LOG_ERROR, "stream url: %s\n", m_PlaybackURL.c_str());

	return m_PlaybackURL.c_str();
}

bool SData::GetChannel(const PVR_CHANNEL &channel, SChannel &myChannel)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
	SChannel &thisChannel = m_channels.at(iChannelPtr);
	if (thisChannel.iUniqueId == (int) channel.iUniqueId)
	{
		myChannel.iUniqueId         = thisChannel.iUniqueId;
		myChannel.bRadio            = thisChannel.bRadio;
		myChannel.iChannelNumber    = thisChannel.iChannelNumber;
		myChannel.iSubChannelNumber = thisChannel.iSubChannelNumber;
		myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
		myChannel.strChannelName    = thisChannel.strChannelName;
		myChannel.strIconPath       = thisChannel.strIconPath;
		myChannel.strStreamURL      = thisChannel.strStreamURL;

		return true;
	}
  }

  return false;
}

int SData::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR SData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    SChannelGroup &group = m_groups.at(iGroupPtr);
    if (group.bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup;
      memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

      xbmcGroup.bIsRadio = bRadio;
      strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    SChannelGroup &myGroup = m_groups.at(iGroupPtr);
    if (!strcmp(myGroup.strGroupName.c_str(),group.strGroupName))
    {
      for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
      {
        int iId = myGroup.members.at(iChannelPtr) - 1;
        if (iId < 0 || iId > (int)m_channels.size() - 1)
          continue;
        SChannel &channel = m_channels.at(iId);
        PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
        memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

        strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
        xbmcGroupMember.iChannelUniqueId  = channel.iUniqueId;
        xbmcGroupMember.iChannelNumber    = channel.iChannelNumber;

        PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (m_iEpgStart == -1)
    m_iEpgStart = iStart;

  time_t iLastEndTime = m_iEpgStart + 1;
  int iAddBroadcastId = 0;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    SChannel &myChannel = m_channels.at(iChannelPtr);
    if (myChannel.iUniqueId != (int) channel.iUniqueId)
      continue;

    while (iLastEndTime < iEnd && myChannel.epg.size() > 0)
    {
      time_t iLastEndTimeTmp = 0;
      for (unsigned int iEntryPtr = 0; iEntryPtr < myChannel.epg.size(); iEntryPtr++)
      {
        SEpgEntry &myTag = myChannel.epg.at(iEntryPtr);

        EPG_TAG tag;
        memset(&tag, 0, sizeof(EPG_TAG));

        tag.iUniqueBroadcastId = myTag.iBroadcastId + iAddBroadcastId;
        tag.strTitle           = myTag.strTitle.c_str();
        tag.iChannelNumber     = myTag.iChannelId;
        tag.startTime          = myTag.startTime + iLastEndTime;
        tag.endTime            = myTag.endTime + iLastEndTime;
        tag.strPlotOutline     = myTag.strPlotOutline.c_str();
        tag.strPlot            = myTag.strPlot.c_str();
        tag.strIconPath        = myTag.strIconPath.c_str();
        tag.iGenreType         = myTag.iGenreType;
        tag.iGenreSubType      = myTag.iGenreSubType;

        iLastEndTimeTmp = tag.endTime;

        PVR->TransferEpgEntry(handle, &tag);
      }

      iLastEndTime = iLastEndTimeTmp;
      iAddBroadcastId += myChannel.epg.size();
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int SData::GetRecordingsAmount(void)
{
  return m_recordings.size();
}

PVR_ERROR SData::GetRecordings(ADDON_HANDLE handle)
{
  for (std::vector<SRecording>::iterator it = m_recordings.begin() ; it != m_recordings.end() ; it++)
  {
    SRecording &recording = *it;

    PVR_RECORDING xbmcRecording;

    xbmcRecording.iDuration     = recording.iDuration;
    xbmcRecording.iGenreType    = recording.iGenreType;
    xbmcRecording.iGenreSubType = recording.iGenreSubType;
    xbmcRecording.recordingTime = recording.recordingTime;

    strncpy(xbmcRecording.strChannelName, recording.strChannelName.c_str(), sizeof(xbmcRecording.strChannelName) - 1);
    strncpy(xbmcRecording.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(xbmcRecording.strPlotOutline) - 1);
    strncpy(xbmcRecording.strPlot,        recording.strPlot.c_str(),        sizeof(xbmcRecording.strPlot) - 1);
    strncpy(xbmcRecording.strRecordingId, recording.strRecordingId.c_str(), sizeof(xbmcRecording.strRecordingId) - 1);
    strncpy(xbmcRecording.strTitle,       recording.strTitle.c_str(),       sizeof(xbmcRecording.strTitle) - 1);
    strncpy(xbmcRecording.strStreamURL,   recording.strStreamURL.c_str(),   sizeof(xbmcRecording.strStreamURL) - 1);
    strncpy(xbmcRecording.strDirectory,   recording.strDirectory.c_str(),   sizeof(xbmcRecording.strDirectory) - 1);

    PVR->TransferRecordingEntry(handle, &xbmcRecording);
  }

  return PVR_ERROR_NO_ERROR;
}

int SData::GetTimersAmount(void)
{
  return m_timers.size();
}

PVR_ERROR SData::GetTimers(ADDON_HANDLE handle)
{
  int i = 0;
  for (std::vector<STimer>::iterator it = m_timers.begin() ; it != m_timers.end() ; it++)
  {
    STimer &timer = *it;

    PVR_TIMER xbmcTimer;
    memset(&xbmcTimer, 0, sizeof(PVR_TIMER));

    xbmcTimer.iClientIndex      = ++i;
    xbmcTimer.iClientChannelUid = timer.iChannelId;
    xbmcTimer.startTime         = timer.startTime;
    xbmcTimer.endTime           = timer.endTime;
    xbmcTimer.state             = timer.state;

    strncpy(xbmcTimer.strTitle, timer.strTitle.c_str(), sizeof(timer.strTitle) - 1);
    strncpy(xbmcTimer.strSummary, timer.strSummary.c_str(), sizeof(timer.strSummary) - 1);

    PVR->TransferTimerEntry(handle, &xbmcTimer);
  }

  return PVR_ERROR_NO_ERROR;
}
