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

  m_bApiInit = false;
  m_bAuthenticated = false;
  m_bProfileLoaded = false;
  m_epgDownloaded = false;
}

SData::~SData(void)
{
	XBMC->Log(LOG_NOTICE, "->~SData()");

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
		XBMC->Log(LOG_DEBUG, "saving cache to user path\n");

		if (!xml_doc.SaveFile(strUSettingsFile)) {
			XBMC->Log(LOG_ERROR, "failed to save %s", strUSettingsFile.c_str());
			return false;
		}

		XBMC->Log(LOG_DEBUG, "reloading cache from user path\n");
		return LoadCache();
	}

	pRootElement = xml_doc.RootElement();
	if (strcmp(pRootElement->Value(), "cache") != 0) {
		XBMC->Log(LOG_ERROR, "invalid xml doc. root tag 'cache' not found\n");
		return false;
	}
	
	pTokenElement = pRootElement->FirstChildElement("token");
	if (!pTokenElement) {
		XBMC->Log(LOG_DEBUG, "\"token\" element not found\n");
	}
	else if (pTokenElement->GetText()) {
		g_token = pTokenElement->GetText();
	}

	XBMC->Log(LOG_DEBUG, "token: %s\n", g_token.c_str());

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
		XBMC->Log(LOG_ERROR, "failed to load \"%s\"\n", strSettingsFile.c_str());
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
		XBMC->Log(LOG_ERROR, "failed to save \"%s\"", strSettingsFile.c_str());
		return false;
	}

	return true;
}

int SData::GetIntValue(Json::Value &value)
{
  const char *tempChar = NULL;
  int tempInt = -1;

  // some json responses have have ints formated as strings
  if (value.isString()) {
    tempChar = value.asCString();
    tempInt = atoi(tempChar);
  }
  else if (value.isInt()) {
    tempInt = value.asInt();
  }

  return tempInt;
}

bool SData::GetIntValueAsBool(Json::Value &value)
{
  return GetIntValue(value) > 0 ? true : false;
}

bool SData::InitAPI()
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  m_bApiInit = false;

  if (!SAPI::Init()) {
    XBMC->Log(LOG_ERROR, "%s: failed to init api\n", __FUNCTION__);
    return false;
  }

  m_bApiInit = true;

  return true;
}

bool SData::LoadProfile()
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  Json::Value parsed;

  m_bProfileLoaded = false;

  if (!SAPI::GetProfile(&parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetProfile failed\n", __FUNCTION__);
    return false;
  }

  m_bProfileLoaded = true;

  return true;
}

bool SData::Authenticate()
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

	Json::Value parsed;

  m_bAuthenticated = false;

  if (!SAPI::Handshake(&parsed) || !LoadProfile()) {
		XBMC->Log(LOG_ERROR, "%s: authentication failed\n", __FUNCTION__);
		return false;
	}

	if (!SaveCache()) {
		return false;
	}

  m_bAuthenticated = true;

	return true;
}

int SData::GetChannelId(const char * strChannelName, const char * strNumber)
{
  std::string concat(strChannelName);
  concat.append(strNumber);

  const char* strString = concat.c_str();
  int iId = 0;
  int c;
  while (c = *strString++)
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}

bool SData::ParseChannels(Json::Value &parsed)
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

	Json::Value tempValue;
	const char *tempChar = NULL;

	for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
		SChannel channel;
    //channel.iUniqueId = GetIntValue((*it)["id"]);
    channel.iUniqueId = GetChannelId((*it)["name"].asCString(), (*it)["number"].asCString());
    channel.iChannelId = GetIntValue((*it)["id"]);

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
		channel.use_http_tmp_link = GetIntValueAsBool((*it)["use_http_tmp_link"]);
		channel.use_load_balancing = GetIntValueAsBool((*it)["use_load_balancing"]);

		/* stream url */
		channel.strStreamURL = "pvr://stream/" + std::to_string(channel.iUniqueId); // "pvr://stream/" causes GetLiveStreamURL to be called

		m_channels.push_back(channel);

		XBMC->Log(LOG_DEBUG, "%s: %d - %s\n", __FUNCTION__, channel.iChannelNumber, channel.strChannelName.c_str());
	}

	return true;
}

bool SData::LoadChannels()
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  if (!m_bApiInit && !InitAPI()) {
    return false;
  }

  if (!m_bAuthenticated && !Authenticate()) {
    return false;
  }

  if (!m_bProfileLoaded && !LoadProfile()) {
    return false;
  }

	Json::Value parsed;
  uint32_t iCurrentPage = 1;
  uint32_t iMaxPages = 1;

	if (!SAPI::GetAllChannels(&parsed) || !ParseChannels(parsed)) {
		XBMC->Log(LOG_NOTICE, "%s: GetAllChannels failed\n", __FUNCTION__);
		return false;
	}

	parsed.clear();

  while (iCurrentPage <= iMaxPages) {
    if (!SAPI::GetOrderedList(iCurrentPage, &parsed) || !ParseChannels(parsed)) {
			XBMC->Log(LOG_NOTICE, "%s: GetOrderedList failed\n", __FUNCTION__);
			return false;
		}

    int iTotalItems = GetIntValue(parsed["js"]["total_items"]);
    int iMaxPageItems = GetIntValue(parsed["js"]["max_page_items"]);
    iMaxPages = static_cast<uint32_t>(ceil((double)iTotalItems / iMaxPageItems));

    iCurrentPage++;

		XBMC->Log(LOG_DEBUG, "%s: iTotalItems: %d | iMaxPageItems: %d | iCurrentPage: %d | iMaxPages: %d\n",
      __FUNCTION__, iTotalItems, iMaxPageItems, iCurrentPage, iMaxPages);
	}

	return true;
}

bool SData::LoadData(void)
{
  if (!LoadCache()) {
    return false;
  }

  if (!g_token.empty()) {
    m_bAuthenticated = true;
  }
	
	/*XBMC->QueueNotification(QUEUE_INFO, "Loading channel list.");
  
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
			XBMC->Log(LOG_NOTICE, "re-authenticating\n");
			if (!Authenticate()) {
				XBMC->QueueNotification(QUEUE_ERROR, "Authentication failed.");
				return PVR_ERROR_SERVER_ERROR;
			}
		}

		XBMC->Log(LOG_NOTICE, "re-attempting to load channels\n");
		if (!LoadChannels()) {
			XBMC->QueueNotification(QUEUE_ERROR, "Unable to load channels.");
			return PVR_ERROR_SERVER_ERROR;
		}
	}*/

	return true;
}

int SData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR SData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

	if (!LoadChannels()) {
		XBMC->QueueNotification(QUEUE_ERROR, "Unable to load channels.");
		return PVR_ERROR_SERVER_ERROR;
	}

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
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

	SChannel *thisChannel = NULL;
	// method 1
	/*const char *cmd;
	const char *streamUrl;*/
	// method 2
	std::string cmd;
	size_t pos;

	for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++) {
		thisChannel = &m_channels.at(iChannelPtr);

		if (thisChannel->iUniqueId == (int)channel.iUniqueId) {
			break;
		}
	}

	if (!thisChannel) {
    XBMC->Log(LOG_ERROR, "%s: channel not found\n", __FUNCTION__);
		return "";
	}

	m_PlaybackURL.clear();

	// /c/player.js#L2198
	if (thisChannel->use_http_tmp_link || thisChannel->use_load_balancing) {
    XBMC->Log(LOG_DEBUG, "%s: getting temp stream url\n", __FUNCTION__);

		Json::Value parsed;

		if (!SAPI::CreateLink(thisChannel->cmd, &parsed)) {
			XBMC->Log(LOG_ERROR, "%s: CreateLink failed\n", __FUNCTION__);
			return "";
		}

    if (parsed["js"].isMember("cmd")) {
      //cmd = parsed["js"]["cmd"].asCString();
      cmd = parsed["js"]["cmd"].asString();
		}
	}
	else {
		cmd = thisChannel->cmd;
	}

	// cmd format
	// (?:ffrt\d*\s|)(.*)

	// method 1
	/*if ((streamUrl = strstr(cmd, " ")) != NULL) {
		m_PlaybackURL.assign(streamUrl + 1);
	}

	m_PlaybackURL.assign(streamUrl + 1);*/

	// method 2
	if ((pos = cmd.find(" ")) != std::string::npos) {
		m_PlaybackURL = cmd.substr(pos + 1);
	}
	else {
		if (cmd.find("http") == 0) {
			m_PlaybackURL = cmd;
		}
	}

	if (m_PlaybackURL.empty()) {
    XBMC->Log(LOG_ERROR, "%s: no stream url found\n", __FUNCTION__);
		return "";
	}

	XBMC->Log(LOG_DEBUG, "%s: stream url: %s\n", __FUNCTION__, m_PlaybackURL.c_str());

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

bool SData::ParseEPG(Json::Value &parsed, time_t iStart, time_t iEnd, int iChannelNumber, ADDON_HANDLE handle)
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  time_t iStartTimestamp;
	time_t iStopTimestamp;

  for (Json::Value::iterator it = parsed.begin(); it != parsed.end(); ++it) {
    iStartTimestamp = GetIntValue((*it)["start_timestamp"]);
		iStopTimestamp = GetIntValue((*it)["stop_timestamp"]);

    if (!(iStartTimestamp > iStart && iStopTimestamp < iEnd)) {
      continue;
    }

    EPG_TAG tag;
		memset(&tag, 0, sizeof(EPG_TAG));

    tag.iUniqueBroadcastId = GetIntValue((*it)["id"]);
    tag.strTitle = (*it)["name"].asCString();
    tag.iChannelNumber = iChannelNumber;
    tag.startTime = iStartTimestamp;
    tag.endTime = iStopTimestamp;
    //tag.strPlotOutline = myTag.strPlotOutline.c_str();
    tag.strPlot = (*it)["descr"].asCString();
    //tag.strIconPath = myTag.strIconPath.c_str();
    //tag.iGenreType = myTag.iGenreType;
    //tag.iGenreSubType = myTag.iGenreSubType;

    PVR->TransferEpgEntry(handle, &tag);
  }

  return true;
}

std::vector<std::string> SData::split(std::string str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str); // Turn the string into a stream.
  std::string tok;

  while (getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }

  return internal;
}

bool SData::LoadEPGForChannel(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  struct tm *tm;
  char buffer[30];
  std::string strFrom;
  std::string strTo;
  uint32_t iCurrentPage = 1;
  uint32_t iMaxPages = 1;
  Json::Value parsed;

  tm = gmtime(&iStart);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
  strFrom = buffer;

  tm = gmtime(&iEnd);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
  strTo = buffer;

  XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %s - %s\n", __FUNCTION__, iStart, iEnd, strFrom.c_str(), strTo.c_str());

  if (m_epgWeek.empty() && !SAPI::GetWeek(&m_epgWeek)) {
    XBMC->Log(LOG_ERROR, "%s: GetWeek failed\n", __FUNCTION__);
    return false;
  }

  for (Json::Value::iterator it = m_epgWeek["js"].begin(); it != m_epgWeek["js"].end(); ++it) {
    std::string strFMysql;
    std::vector<std::string> dateSplit;
    struct tm tmDate = { 0 };
    time_t iTimeStamp;

    strFMysql = (*it)["f_mysql"].asString();
    dateSplit = split(strFMysql, '-');

    tmDate.tm_year = atoi(dateSplit[0].c_str()) - 1900;
    tmDate.tm_mon = atoi(dateSplit[1].c_str()) - 1;
    tmDate.tm_mday = atoi(dateSplit[2].c_str());
    tmDate.tm_isdst = 0;
    iTimeStamp = mktime(&tmDate);

    XBMC->Log(LOG_DEBUG, "%s: %s - %d\n", __FUNCTION__, strFMysql.c_str(), iTimeStamp);

    // check if week is within range
    if (!(iTimeStamp > iStart && iTimeStamp < iEnd)) {
      continue;
    }

    iCurrentPage = 1;
    iMaxPages = 1;

    while (iCurrentPage <= iMaxPages) {
      if (!SAPI::GetSimpleDataTable(channel.iChannelId, strFMysql, iCurrentPage, &parsed)
        || !ParseEPG(parsed["js"]["data"], iStart, iEnd, channel.iChannelNumber, handle))
      {
        XBMC->Log(LOG_ERROR, "%s: GetSimpleDataTable failed\n", __FUNCTION__);
        return false;
      }

      int iTotalItems = GetIntValue(parsed["js"]["total_items"]);
      int iMaxPageItems = GetIntValue(parsed["js"]["max_page_items"]);
      iMaxPages = static_cast<uint32_t>(ceil((double)iTotalItems / iMaxPageItems));

      iCurrentPage++;

      XBMC->Log(LOG_DEBUG, "%s: iTotalItems: %d | iMaxPageItems: %d | iCurrentPage: %d | iMaxPages: %d\n",
        __FUNCTION__, iTotalItems, iMaxPageItems, iCurrentPage, iMaxPages);
    }
  }

  return true;
}

bool SData::LoadEPGForChannel2(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  struct tm *tm;
  char buffer[30];
  std::string strFrom;
  std::string strTo;
  Json::Value parsed;

  tm = gmtime(&iStart);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
  strFrom = buffer;

  tm = gmtime(&iEnd);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
  strTo = buffer;

  XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %s - %s\n", __FUNCTION__, iStart, iEnd, strFrom.c_str(), strTo.c_str());

  if (m_epgWeek.empty() && !SAPI::GetWeek(&m_epgWeek)) {
    XBMC->Log(LOG_ERROR, "%s: GetWeek failed\n", __FUNCTION__);
    return false;
  }

  for (Json::Value::iterator it = m_epgWeek["js"].begin(); it != m_epgWeek["js"].end(); ++it) {
    std::string strFMysql;
    std::vector<std::string> dateSplit;
    struct tm tmDate = { 0 };
    time_t iTimeStamp;

    strFMysql = (*it)["f_mysql"].asString();
    dateSplit = split(strFMysql, '-');

    tmDate.tm_year = atoi(dateSplit[0].c_str()) - 1900;
    tmDate.tm_mon = atoi(dateSplit[1].c_str()) - 1;
    tmDate.tm_mday = atoi(dateSplit[2].c_str());
    tmDate.tm_isdst = 0;
    iTimeStamp = mktime(&tmDate);

    XBMC->Log(LOG_DEBUG, "%s: %s - %d\n", __FUNCTION__, strFMysql.c_str(), iTimeStamp);

    // check if week is within range
    if (!(iTimeStamp > iStart && iTimeStamp < iEnd)) {
      continue;
    }

    // 1) requires ch_id but returns other ch_ids in the result
		// 2) all pages for the same ch_id return the same result
		// 3) ch_ids seem to be in specific groups
    if (!SAPI::GetDataTable(channel.iChannelId, strFrom, strTo, 1, &parsed)) {
      XBMC->Log(LOG_ERROR, "%s: GetDataTable failed\n", __FUNCTION__);
      return false;
    }

    int channelId;
    for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
      channelId = GetIntValue((*it)["ch_id"]);
      if (channelId != channel.iChannelId) {
        continue;
      }

      if (!ParseEPG((*it)["epg"], iStart, iEnd, channel.iChannelNumber, handle)) {
        XBMC->Log(LOG_ERROR, "%s: ParseEPG failed\n", __FUNCTION__);
        return false;
      }

      // epg loaded for channel. no need to look further
      return true;
    }
  }

  return true;
}

std::string SData::GetEPGCachePath()
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);
  
  return std::string(g_strUserPath + PATH_SEPARATOR_CHAR + "epg" + PATH_SEPARATOR_CHAR);
}

bool SData::DownloadEPG(time_t iStart, time_t iEnd, SChannel &channel)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  struct tm *tm;
  char buffer[30];
  std::string strFrom;
  std::string strTo;
  uint32_t iCurrentPage = 1;
  uint32_t iMaxPages = 1;
  std::string strEpgCachePath;
  std::string strFilePath;
  Json::Value parsed;

  m_epgDownloaded = false;

  tm = gmtime(&iStart);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
  strFrom = buffer;

  tm = gmtime(&iEnd);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
  strTo = buffer;

  XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %s - %s\n", __FUNCTION__, iStart, iEnd, strFrom.c_str(), strTo.c_str());
  
  strEpgCachePath = GetEPGCachePath();

  if (!XBMC->DirectoryExists(strEpgCachePath.c_str()) && !XBMC->CreateDirectoryA(strEpgCachePath.c_str())) {
    XBMC->Log(LOG_ERROR, "%s: CreateDirectoryA failed: %s\n", __FUNCTION__, strEpgCachePath.c_str());
    return false;
  }

  while (iCurrentPage <= iMaxPages) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%sepg-%d",
      strEpgCachePath.c_str(), iCurrentPage);
    strFilePath = buffer;

    XBMC->Log(LOG_DEBUG, "%s: file path: %s\n", __FUNCTION__, strFilePath.c_str());

    // don't download it if it's already cached
    /*if (XBMC->FileExists(strFilePath.c_str(), false)) {
      XBMC->Log(LOG_NOTICE, "%s: epg data already cached\n", __FUNCTION__);
      continue;
    }

    if (!SAPI::GetDataTable(channel.iChannelId, strFrom, strTo, iCurrentPage, &parsed, strFilePath)) {
      XBMC->Log(LOG_ERROR, "%s: GetDataTable failed\n", __FUNCTION__);
      return false;
    }*/

    int iTotalItems = GetIntValue(parsed["js"]["total_items"]);
    int iMaxPageItems = GetIntValue(parsed["js"]["max_page_items"]);
    //iMaxPages = static_cast<uint32_t>(ceil((double)iTotalItems / iMaxPageItems));

    iCurrentPage++;

    XBMC->Log(LOG_DEBUG, "%s: iTotalItems: %d | iMaxPageItems: %d | iCurrentPage: %d | iMaxPages: %d\n",
      __FUNCTION__, iTotalItems, iMaxPageItems, iCurrentPage, iMaxPages);
  }

  m_epgDownloaded = true;

  return true;
}

bool SData::LoadEPGFromFile(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  int i = 1;
  std::string strEpgCachePath;
  std::string strFilePath;

  strEpgCachePath = GetEPGCachePath();

  while (true) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%sepg-%d", strEpgCachePath.c_str(), i++);
    strFilePath = buffer;

    XBMC->Log(LOG_DEBUG, "%s: file path: %s\n", __FUNCTION__, strFilePath.c_str());

    if (!XBMC->FileExists(strFilePath.c_str(), false)) {
      break;
    }

    void* hFile = XBMC->OpenFile(strFilePath.c_str(), 0);
    if (hFile == NULL) {
      XBMC->Log(LOG_ERROR, "%s: OpenFile failed\n", __FUNCTION__);
      return false;
    }

    std::string contents;
    contents.clear();
    while (XBMC->ReadFileString(hFile, buffer, 1023)) {
      contents.append(buffer);
    }

    XBMC->CloseFile(hFile);

    Json::Reader reader;
    Json::Value parsed;
    if (!reader.parse(contents, parsed)) {
      XBMC->Log(LOG_ERROR, "%s: parsing failed\n", __FUNCTION__);
      return false;
    }

    int channelId;
    for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
      channelId = GetIntValue((*it)["ch_id"]);
      if (channelId != channel.iChannelId) {
        continue;
      }

      if (!ParseEPG((*it)["epg"], iStart, iEnd, channel.iChannelNumber, handle)) {
        XBMC->Log(LOG_ERROR, "%s: ParseEPG failed\n", __FUNCTION__);
        return false;
      }

      // epg loaded for channel. no need to look in other files
      return true;
    }
  }

  return true;
}

bool SData::LoadEPGForChannel3(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

	uint32_t iPeriod;
	char strChannelId[25];

	iPeriod = (iEnd - iStart) / 3600;

	if (m_epgData.empty() && !SAPI::GetEPGInfo(iPeriod, &m_epgData)) {
		XBMC->Log(LOG_ERROR, "%s: GetEPGInfo failed\n", __FUNCTION__);
		return false;
	}

	sprintf(strChannelId, "%d", channel.iChannelId);

	if (!ParseEPG(m_epgData["js"]["data"][strChannelId], iStart, iEnd, channel.iChannelNumber, handle)) {
		XBMC->Log(LOG_ERROR, "%s: ParseEPG failed\n", __FUNCTION__);
		return false;
	}

	return true;
}

PVR_ERROR SData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
	XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  SChannel *thisChannel = NULL;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++) {
    thisChannel = &m_channels.at(iChannelPtr);

    if (thisChannel->iUniqueId == (int)channel.iUniqueId) {
      break;
    }
  }

  if (!thisChannel) {
    XBMC->Log(LOG_ERROR, "%s: channel not found\n", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

	XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %d - %s\n",
		__FUNCTION__, iStart, iEnd, thisChannel->iChannelNumber, thisChannel->strChannelName.c_str());

  /*if (!LoadEPGForChannel(*thisChannel, iStart, iEnd, handle)) {
    return PVR_ERROR_SERVER_ERROR;
  }*/

  /*if (/*!m_epgDownloaded &&* !DownloadEPG(iStart, iEnd, *thisChannel)) {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (!LoadEPGFromFile(*thisChannel, iStart, iEnd, handle)) {
    return PVR_ERROR_SERVER_ERROR;
  }*/

	/*if (!LoadEPGForChannel2(*thisChannel, iStart, iEnd, handle)) {
		return PVR_ERROR_SERVER_ERROR;
	}*/

	if (!LoadEPGForChannel3(*thisChannel, iStart, iEnd, handle)) {
		return PVR_ERROR_SERVER_ERROR;
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
