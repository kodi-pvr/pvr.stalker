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

  m_bInitialized    = false;
  m_bApiInit        = false;
  m_bAuthenticated  = false;
  m_bProfileLoaded  = false;
}

SData::~SData(void)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  m_channels.clear();
  m_channelGroups.clear();
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

bool SData::Initialize()
{
  m_bInitialized = false;

  if (!m_bApiInit && !InitAPI()) {
    return m_bInitialized;
  }

  if (!m_bAuthenticated && !Authenticate()) {
    return m_bInitialized;
  }

  if (!m_bProfileLoaded && !LoadProfile()) {
    return m_bInitialized;
  }

  m_bInitialized = true;

  return m_bInitialized;
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
  const char *strTemp = NULL;

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
    strTemp = (*it)["number"].asCString();
    channel.iChannelNumber = atoi(strTemp);

    /* sub channel number */
    channel.iSubChannelNumber = 0;

    /* CAID */
    channel.iEncryptionSystem = 0;

    /* icon path */
    strTemp = (*it)["logo"].asCString();
    channel.strIconPath = strlen(strTemp) == 0 ? "" : std::string(g_strApiBasePath + "misc/logos/120/" + strTemp).c_str();

    channel.cmd = (*it)["cmd"].asString();
    channel.tv_genre_id = (*it)["tv_genre_id"].asString();
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

  if (!m_bInitialized && !Initialize()) {
    return false;
  }

  Json::Value parsed;
  uint32_t iCurrentPage = 1;
  uint32_t iMaxPages = 1;

  if (!SAPI::GetAllChannels(&parsed) || !ParseChannels(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetAllChannels failed\n", __FUNCTION__);
    return false;
  }

  parsed.clear();

  while (iCurrentPage <= iMaxPages) {
    if (!SAPI::GetOrderedList(iCurrentPage, &parsed) || !ParseChannels(parsed)) {
      XBMC->Log(LOG_ERROR, "%s: GetOrderedList failed\n", __FUNCTION__);
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

bool SData::ParseChannelGroups(Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  try {
    for (Json::Value::iterator it = parsed["js"].begin(); it != parsed["js"].end(); ++it) {
      SChannelGroup channelGroup;
      channelGroup.strGroupName = (*it)["title"].asString();
      channelGroup.strGroupName[0] = toupper(channelGroup.strGroupName[0]);
      channelGroup.bRadio = false;
      channelGroup.strId = (*it)["id"].asString();
      channelGroup.strAlias = (*it)["alias"].asString();

      m_channelGroups.push_back(channelGroup);

      XBMC->Log(LOG_DEBUG, "%s: %s - %s\n", __FUNCTION__, channelGroup.strId.c_str(), channelGroup.strGroupName.c_str());
    }
  }
  catch (...) {
    return false;
  }

  return true;
}

bool SData::LoadChannelGroups()
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  if (!m_bInitialized && !Initialize()) {
    return false;
  }

  Json::Value parsed;

  // genres are channel groups
  if (!SAPI::GetGenres(&parsed) || !ParseChannelGroups(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetGenres|ParseChannelGroups failed\n", __FUNCTION__);
    return false;
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
  return m_channelGroups.size();
}

PVR_ERROR SData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  if (!LoadChannelGroups()) {
    XBMC->QueueNotification(QUEUE_ERROR, "Unable to load channel groups.");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (std::vector<SChannelGroup>::iterator group = m_channelGroups.begin(); group != m_channelGroups.end(); ++group) {
    // exclude group id '*' (all)
    if (group->strId.compare("*") == 0) {
      continue;
    }

    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(tag));

    strncpy(tag.strGroupName, group->strGroupName.c_str(), sizeof(tag.strGroupName) - 1);
    tag.bIsRadio = group->bRadio;

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  XBMC->Log(LOG_DEBUG, "%s\n", __FUNCTION__);

  SChannelGroup *channelGroup = NULL;

  for (std::vector<SChannelGroup>::iterator it = m_channelGroups.begin(); it != m_channelGroups.end(); ++it) {
    if (strcmp(it->strGroupName.c_str(), group.strGroupName) == 0) {
      channelGroup = &(*it);
      break;
    }
  }

  if (!channelGroup) {
    XBMC->Log(LOG_ERROR, "%s: channel not found\n", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (std::vector<SChannel>::iterator channel = m_channels.begin(); channel != m_channels.end(); ++channel) {
    if (channel->tv_genre_id.compare(channelGroup->strId) == 0) {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(tag));

      strncpy(tag.strGroupName, channelGroup->strGroupName.c_str(), sizeof(tag.strGroupName) - 1);
      tag.iChannelUniqueId = channel->iUniqueId;
      tag.iChannelNumber = channel->iChannelNumber;

      PVR->TransferChannelGroupMember(handle, &tag);
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
