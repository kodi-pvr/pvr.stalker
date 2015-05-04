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

#include <cmath>
#include <map>
#include <iterator>

#include "tinyxml.h"
#include "platform/os.h"

#include "SAPI.h"

using namespace ADDON;

SData::SData(void)
{
  m_bInitedApi        = false;
  m_bDidHandshake   = false;
  m_bLoadedProfile  = false;
  m_bInitialized    = false;
}

SData::~SData(void)
{
  m_channelGroups.clear();
  m_channels.clear();
}

std::string SData::GetFilePath(std::string strPath, bool bUserPath) const
{
  return (bUserPath ? g_strUserPath : g_strClientPath) + PATH_SEPARATOR_CHAR + strPath;
}

bool SData::LoadCache()
{
  std::string strUCacheFile;
  bool bUCacheFileExists;
  std::string strCacheFile;
  TiXmlDocument xml_doc;
  TiXmlElement *pRootElement = NULL;
  TiXmlElement *pTokenElement = NULL;

  strUCacheFile = GetFilePath("cache.xml");
  bUCacheFileExists = XBMC->FileExists(strUCacheFile.c_str(), false);
  strCacheFile = bUCacheFileExists ? strUCacheFile : GetFilePath("cache.xml", false);

  if (!xml_doc.LoadFile(strCacheFile)) {
    XBMC->Log(LOG_ERROR, "failed to load: %s", strCacheFile.c_str());
    return false;
  }

  if (!bUCacheFileExists) {
    XBMC->Log(LOG_DEBUG, "saving cache to user path");

    if (!xml_doc.SaveFile(strUCacheFile)) {
      XBMC->Log(LOG_ERROR, "failed to save %s", strUCacheFile.c_str());
      return false;
    }

    XBMC->Log(LOG_DEBUG, "reloading cache from user path");
    return LoadCache();
  }

  pRootElement = xml_doc.RootElement();
  if (strcmp(pRootElement->Value(), "cache") != 0) {
    XBMC->Log(LOG_ERROR, "invalid xml doc. root tag 'cache' not found");
    return false;
  }
  
  pTokenElement = pRootElement->FirstChildElement("token");
  if (!pTokenElement) {
    XBMC->Log(LOG_DEBUG, "\"token\" element not found");
  }
  else if (pTokenElement->GetText()) {
    g_token = pTokenElement->GetText();
  }

  XBMC->Log(LOG_DEBUG, "token: %s", g_token.c_str());

  return true;
}

bool SData::SaveCache()
{
  std::string strCacheFile;
  TiXmlDocument xml_doc;
  TiXmlElement *pRootElement = NULL;
  TiXmlElement *pTokenElement = NULL;

  strCacheFile = GetFilePath("cache.xml");

  if (!xml_doc.LoadFile(strCacheFile)) {
    XBMC->Log(LOG_ERROR, "failed to load \"%s\"", strCacheFile.c_str());
    return false;
  }

  pRootElement = xml_doc.RootElement();
  if (strcmp(pRootElement->Value(), "cache") != 0) {
    XBMC->Log(LOG_ERROR, "invalid xml doc. root tag 'cache' not found");
    return false;
  }

  pTokenElement = pRootElement->FirstChildElement("token");
  pTokenElement->Clear();
  pTokenElement->LinkEndChild(new TiXmlText(g_token));

  strCacheFile = GetFilePath("cache.xml");
  if (!xml_doc.SaveFile(strCacheFile)) {
    XBMC->Log(LOG_ERROR, "failed to save \"%s\"", strCacheFile.c_str());
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
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_bInitedApi = false;

  if (!SAPI::Init()) {
    XBMC->Log(LOG_ERROR, "%s: failed to init api", __FUNCTION__);
    return false;
  }

  memset(&m_identity, 0, sizeof(m_identity));
  m_identity.mac = g_strMac.c_str();
  m_identity.lang = "en";
  m_identity.time_zone = g_strTimeZone.c_str();
  m_identity.auth_token = g_token.c_str();

  m_bInitedApi = true;

  return true;
}

bool SData::DoHandshake()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value parsed;

  m_bDidHandshake = false;

  if (!SAPI::Handshake(&m_identity, &parsed)) {
    XBMC->Log(LOG_ERROR, "%s: Handshake failed", __FUNCTION__);
    return false;
  }
  
  if (parsed["js"].isMember("token"))
    m_identity.auth_token = parsed["js"]["token"].asCString();

  XBMC->Log(LOG_DEBUG, "auth_token: %s", m_identity.auth_token);
  
  m_bAuthTokenNotValid = parsed["js"].isMember("not_valid")
    ? GetIntValueAsBool(parsed["js"]["not_valid"])
    : 0;

  m_bDidHandshake = true;

  return true;
}

bool SData::LoadProfile()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value parsed;

  m_bLoadedProfile = false;

  if (!SAPI::GetProfile(&m_identity, m_bAuthTokenNotValid, &parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetProfile failed", __FUNCTION__);
    return false;
  }
  
  m_profile.store_auth_data_on_stb = parsed["js"].isMember("store_auth_data_on_stb")
    ? GetIntValueAsBool(parsed["js"]["store_auth_data_on_stb"])
    : false;
  m_profile.status = parsed["js"].isMember("status")
    ? GetIntValue(parsed["js"]["status"])
    : -1;
  
  m_profile.block_msg = parsed["js"].isMember("msg")
    ? (char *)parsed["js"]["msg"].asCString()
    : "";
  
  m_profile.block_msg = parsed["js"].isMember("block_msg")
    ? (char *)parsed["js"]["block_msg"].asCString()
    : "";
  
  if (m_profile.store_auth_data_on_stb && !SaveCache())
    return false;
  
  switch (m_profile.status) {
    case 0:
      break;
    case 2:
      //TODO auth
    case 1:
    default:
      XBMC->Log(LOG_ERROR, "%s: status=%i | msg=%s | block_msg=%s",
              __FUNCTION__, m_profile.status,
              (const char *)m_profile.msg,
              (const char *)m_profile.block_msg);
      return false;
  }

  m_bLoadedProfile = true;

  return true;
}

bool SData::Initialize()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_bInitialized = false;

  if (!m_bInitedApi && !InitAPI()) {
    return m_bInitialized;
  }

  if (!m_bDidHandshake && !DoHandshake()) {
    return m_bInitialized;
  }

  if (!m_bLoadedProfile && !LoadProfile()) {
    return m_bInitialized;
  }

  m_bInitialized = true;

  return m_bInitialized;
}

bool SData::ParseEPG(Json::Value &parsed, time_t iStart, time_t iEnd, int iChannelNumber, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

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
    tag.strPlot = (*it)["descr"].asCString();

    PVR->TransferEpgEntry(handle, &tag);
  }

  return true;
}

bool SData::ParseEPGXMLTV(int iChannelNumber, std::string &strChannelName, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  std::string strChanNum;
  Channel *chan = NULL;
  
  std::ostringstream oss;
  oss << iChannelNumber;
  strChanNum = oss.str();
  
  chan = m_xmltv->GetChannelById(strChanNum);
  if (!chan)
    chan = m_xmltv->GetChannelByDisplayName(strChannelName);
  
  if (!chan) {
    XBMC->Log(LOG_ERROR, "%s: channel \"%s\" not found", __FUNCTION__, strChanNum.c_str());
    return true; // don't fail
  }
  
  for (std::vector<Programme>::iterator it = chan->programmes.begin(); it != chan->programmes.end(); ++it) {
    if (!(it->start > iStart && it->stop < iEnd)) {
      continue;
    }

    EPG_TAG tag;
    memset(&tag, 0, sizeof(EPG_TAG));

    tag.iUniqueBroadcastId = it->iBroadcastId;
    tag.strTitle = it->strTitle.c_str();
    tag.iChannelNumber = iChannelNumber;
    tag.startTime = it->start;
    tag.endTime = it->stop;
    tag.strPlot = it->strDesc.c_str();
    
    std::string strYear = it->strDate.substr(0, 4); // only need the year;
    //TODO move to utils
    int value = 0;
    try {
      std::istringstream iss(strYear);
      iss >> value;
    }
    catch (...) {
      
    }
    tag.iYear = value;
    
    tag.iGenreType = m_xmltv->EPGGenreByCategory(it->categories);
    if (tag.iGenreType == EPG_GENRE_USE_STRING) {
      //TODO move to utils
      std::ostringstream oss;
      if (!it->categories.empty()) {
        std::copy(it->categories.begin(), it->categories.end() - 1, 
          std::ostream_iterator<std::string>(oss, ", "));
        oss << it->categories.back();
      }
      tag.strGenreDescription = oss.str().c_str();
    }
    
    tag.firstAired = it->previouslyShown;
    
    std::string strStarRating = it->strStarRating.substr(0, 1);
    //TODO move to utils
    value = 0;
    try {
      std::istringstream iss(strStarRating);
      iss >> value;
    }
    catch (...) {
      
    }
    tag.iStarRating = value;
    
    tag.iEpisodeNumber = it->iEpisodeNumber;
    tag.strEpisodeName = it->strSubTitle.c_str();
    
    PVR->TransferEpgEntry(handle, &tag);
  }
  
  return true;
}

bool SData::LoadEPGForChannel(SChannel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  uint32_t iPeriod;
  char strChannelId[25];

  iPeriod = (iEnd - iStart) / 3600;

  if (m_epgData.empty() && !SAPI::GetEPGInfo(iPeriod, &m_identity, &m_epgData)) {
    XBMC->Log(LOG_ERROR, "%s: GetEPGInfo failed", __FUNCTION__);
    return false;
  }
  
  if (m_xmltv && !m_xmltv->bLoaded && !m_xmltv->Parse(GetFilePath("xmltv.xml"))) {
    XBMC->Log(LOG_ERROR, "%s: XMLTV Parse failed", __FUNCTION__);
    return false;
  }

  sprintf(strChannelId, "%d", channel.iChannelId);

  if (!ParseEPG(m_epgData["js"]["data"][strChannelId], iStart, iEnd, channel.iChannelNumber, handle) ||
    !ParseEPGXMLTV(channel.iChannelNumber, channel.strChannelName, iStart, iEnd, handle)) {
    XBMC->Log(LOG_ERROR, "%s: ParseEPG failed", __FUNCTION__);
    return false;
  }

  return true;
}

bool SData::ParseChannelGroups(Json::Value &parsed)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  try {
    for (Json::Value::iterator it = parsed["js"].begin(); it != parsed["js"].end(); ++it) {
      SChannelGroup channelGroup;
      channelGroup.strGroupName = (*it)["title"].asString();
      channelGroup.strGroupName[0] = toupper(channelGroup.strGroupName[0]);
      channelGroup.bRadio = false;
      channelGroup.strId = (*it)["id"].asString();
      channelGroup.strAlias = (*it)["alias"].asString();

      m_channelGroups.push_back(channelGroup);

      XBMC->Log(LOG_DEBUG, "%s: %s - %s", __FUNCTION__, channelGroup.strId.c_str(), channelGroup.strGroupName.c_str());
    }
  }
  catch (...) {
    return false;
  }

  return true;
}

bool SData::LoadChannelGroups()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!m_bInitialized && !Initialize()) {
    return false;
  }

  Json::Value parsed;

  // genres are channel groups
  if (!SAPI::GetGenres(&m_identity, &parsed) || !ParseChannelGroups(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetGenres|ParseChannelGroups failed", __FUNCTION__);
    return false;
  }

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
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value tempValue;
  const char *strTemp = NULL;

  for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
    SChannel channel;
    //channel.iUniqueId = GetIntValue((*it)["id"]);
    channel.iUniqueId = GetChannelId((*it)["name"].asCString(), (*it)["number"].asCString());

    channel.bRadio = false;

    strTemp = (*it)["number"].asCString();
    channel.iChannelNumber = atoi(strTemp);

    channel.strChannelName = (*it)["name"].asString();

    char strStreamURL[256];
    sprintf(strStreamURL, "pvr://stream/%d", channel.iUniqueId); // "pvr://stream/" causes GetLiveStreamURL to be called
    channel.strStreamURL = strStreamURL;

    strTemp = (*it)["logo"].asCString();
    channel.strIconPath = strlen(strTemp) == 0 ? "" : std::string(g_strApiBasePath + "misc/logos/120/" + strTemp).c_str();

    channel.iChannelId = GetIntValue((*it)["id"]);
    channel.strCmd = (*it)["cmd"].asString();
    channel.strTvGenreId = (*it)["tv_genre_id"].asString();
    channel.bUseHttpTmpLink = GetIntValueAsBool((*it)["use_http_tmp_link"]);
    channel.bUseLoadBalancing = GetIntValueAsBool((*it)["use_load_balancing"]);
    
    m_channels.push_back(channel);

    XBMC->Log(LOG_DEBUG, "%s: %d - %s", __FUNCTION__, channel.iChannelNumber, channel.strChannelName.c_str());
  }

  return true;
}

bool SData::LoadChannels()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!m_bInitialized && !Initialize()) {
    return false;
  }

  Json::Value parsed;
  std::string genre = "10";
  uint32_t iCurrentPage = 1;
  uint32_t iMaxPages = 1;

  if (!SAPI::GetAllChannels(&m_identity, &parsed) || !ParseChannels(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: GetAllChannels failed", __FUNCTION__);
    return false;
  }

  parsed.clear();

  while (iCurrentPage <= iMaxPages) {
    if (!SAPI::GetOrderedList(genre, iCurrentPage, &m_identity, &parsed) || !ParseChannels(parsed)) {
      XBMC->Log(LOG_ERROR, "%s: GetOrderedList failed", __FUNCTION__);
      return false;
    }

    int iTotalItems = GetIntValue(parsed["js"]["total_items"]);
    int iMaxPageItems = GetIntValue(parsed["js"]["max_page_items"]);
    iMaxPages = static_cast<uint32_t>(ceil((double)iTotalItems / iMaxPageItems));

    iCurrentPage++;

    XBMC->Log(LOG_DEBUG, "%s: iTotalItems: %d | iMaxPageItems: %d | iCurrentPage: %d | iMaxPages: %d",
      __FUNCTION__, iTotalItems, iMaxPageItems, iCurrentPage, iMaxPages);
  }

  return true;
}

bool SData::LoadData(void)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!LoadCache()) {
    return false;
  }

  if (!g_token.empty()) {
    m_bDidHandshake = true;
  }

  return true;
}

PVR_ERROR SData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SChannel *thisChannel = NULL;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++) {
    thisChannel = &m_channels.at(iChannelPtr);

    if (thisChannel->iUniqueId == (int)channel.iUniqueId) {
      break;
    }
  }

  if (!thisChannel) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %d - %s",
    __FUNCTION__, iStart, iEnd, thisChannel->iChannelNumber, thisChannel->strChannelName.c_str());

  if (!LoadEPGForChannel(*thisChannel, iStart, iEnd, handle)) {
    return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

int SData::GetChannelGroupsAmount(void)
{
  return m_channelGroups.size();
}

PVR_ERROR SData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (bRadio) {
    return PVR_ERROR_NO_ERROR;
  }

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
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SChannelGroup *channelGroup = NULL;

  for (std::vector<SChannelGroup>::iterator it = m_channelGroups.begin(); it != m_channelGroups.end(); ++it) {
    if (strcmp(it->strGroupName.c_str(), group.strGroupName) == 0) {
      channelGroup = &(*it);
      break;
    }
  }

  if (!channelGroup) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (std::vector<SChannel>::iterator channel = m_channels.begin(); channel != m_channels.end(); ++channel) {
    if (channel->strTvGenreId.compare(channelGroup->strId) == 0) {
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

int SData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR SData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (bRadio) {
    return PVR_ERROR_NO_ERROR;
  }

  if (!LoadChannels()) {
    XBMC->QueueNotification(QUEUE_ERROR, "Unable to load channels.");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (std::vector<SChannel>::iterator channel = m_channels.begin(); channel != m_channels.end(); ++channel) {
    PVR_CHANNEL tag;
    memset(&tag, 0, sizeof(tag));

    tag.iUniqueId = channel->iUniqueId;
    tag.bIsRadio = channel->bRadio;
    tag.iChannelNumber = channel->iChannelNumber;
    strncpy(tag.strChannelName, channel->strChannelName.c_str(), sizeof(tag.strChannelName) - 1);
    strncpy(tag.strStreamURL, channel->strStreamURL.c_str(), sizeof(tag.strStreamURL) - 1);
    strncpy(tag.strIconPath, channel->strIconPath.c_str(), sizeof(tag.strIconPath) - 1);
    tag.bIsHidden = false;

    PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

const char* SData::GetChannelStreamURL(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SChannel *thisChannel = NULL;
  std::string strCmd;
  size_t pos;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++) {
    thisChannel = &m_channels.at(iChannelPtr);

    if (thisChannel->iUniqueId == (int)channel.iUniqueId) {
      break;
    }
  }

  if (!thisChannel) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return "";
  }

  m_PlaybackURL.clear();

  // /c/player.js#L2198
  if (thisChannel->bUseHttpTmpLink || thisChannel->bUseLoadBalancing) {
    XBMC->Log(LOG_DEBUG, "%s: getting temp stream url", __FUNCTION__);

    Json::Value parsed;

    if (!SAPI::CreateLink(thisChannel->strCmd, &m_identity, &parsed)) {
      XBMC->Log(LOG_ERROR, "%s: CreateLink failed", __FUNCTION__);
      return "";
    }

    if (parsed["js"].isMember("cmd")) {
      strCmd = parsed["js"]["cmd"].asString();
    }
  }
  else {
    strCmd = thisChannel->strCmd;
  }

  // cmd format
  // (?:ffrt\d*\s|)(.*)
  if ((pos = strCmd.find(" ")) != std::string::npos) {
    m_PlaybackURL = strCmd.substr(pos + 1);
  }
  else {
    if (strCmd.find("http") == 0) {
      m_PlaybackURL = strCmd;
    }
  }

  if (m_PlaybackURL.empty()) {
    XBMC->Log(LOG_ERROR, "%s: no stream url found", __FUNCTION__);
    return "";
  }

  XBMC->Log(LOG_DEBUG, "%s: stream url: %s", __FUNCTION__, m_PlaybackURL.c_str());

  return m_PlaybackURL.c_str();
}
