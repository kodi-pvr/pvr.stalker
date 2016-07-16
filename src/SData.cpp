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

#include "SData.h"

#include <cmath>

#include "p8-platform/util/StringUtils.h"
#include "p8-platform/util/timeutils.h"
#include "p8-platform/util/util.h"

#include "libstalkerclient/util.h"
#include "Utils.h"

#define SERROR_MSG_UNKNOWN              30501
#define SERROR_MSG_INITIALIZE           30502
#define SERROR_MSG_API                  30503
#define SERROR_MSG_AUTHENTICATION       30504
#define SERROR_MSG_LOAD_CHANNELS        30505
#define SERROR_MSG_LOAD_CHANNEL_GROUPS  30506
#define SERROR_MSG_LOAD_EPG             30507
#define SERROR_MSG_STREAM_URL           30508
#define SERROR_MSG_AUTHORIZATION        30509
#define MSG_RE_AUTHENTICATED            30510

using namespace ADDON;
using namespace P8PLATFORM;

SData::SData(void) : Base::Cache()
{
  m_bInitedApi          = false;
  m_bTokenManuallySet   = false;
  m_bAuthenticated      = false;
  m_iLastEpgAccessTime  = 0;
  m_iNextEpgLoadTime    = 0;
  m_watchdog            = NULL;
  m_xmltv               = new XMLTV;
  m_api                 = new SC::SAPI;
  m_channelManager      = new SC::ChannelManager;

  sc_identity_defaults(&m_identity);
  sc_stb_profile_defaults(&m_profile);
}

SData::~SData(void)
{
  if (m_watchdog && !m_watchdog->StopThread())
    XBMC->Log(LOG_DEBUG, "%s: %s", __FUNCTION__, "failed to stop Watchdog");

  SAFE_DELETE(m_watchdog);
  SAFE_DELETE(m_xmltv);
  SAFE_DELETE(m_api);
  SAFE_DELETE(m_channelManager);
}

void SData::QueueErrorNotification(SError error)
{
  int iErrorMsg = 0;

  switch (error) {
    case SERROR_UNKNOWN:
      if (m_strLastUnknownError.empty()) {
        iErrorMsg = SERROR_MSG_UNKNOWN;
        break;
      }
      XBMC->QueueNotification(QUEUE_ERROR, m_strLastUnknownError.c_str());
      m_strLastUnknownError.clear();
      break;
    case SERROR_INITIALIZE:
      iErrorMsg = SERROR_MSG_INITIALIZE;
      break;
    case SERROR_API:
      iErrorMsg = SERROR_MSG_API;
      break;
    case SERROR_AUTHENTICATION:
      iErrorMsg = SERROR_MSG_AUTHENTICATION;
      break;
    case SERROR_LOAD_CHANNELS:
      iErrorMsg = SERROR_MSG_LOAD_CHANNELS;
      break;
    case SERROR_LOAD_CHANNEL_GROUPS:
      iErrorMsg = SERROR_MSG_LOAD_CHANNEL_GROUPS;
      break;
    case SERROR_LOAD_EPG:
      iErrorMsg = SERROR_MSG_LOAD_EPG;
      break;
    case SERROR_STREAM_URL:
      iErrorMsg = SERROR_MSG_STREAM_URL;
      break;
    case SERROR_AUTHORIZATION:
      iErrorMsg = SERROR_MSG_AUTHORIZATION;
      break;
  }

  if (iErrorMsg > 0)
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(iErrorMsg));
}

bool SData::LoadCache()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::string cacheFile;
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL;
  xmlNodePtr node = NULL;
  xmlNodePtr portalsNode = NULL;
  xmlNodePtr portalNode = NULL;
  std::string portalNum = Utils::ToString(g_iActivePortal);

  cacheFile = Utils::GetFilePath("cache.xml");

  if (!Open(cacheFile, doc, rootNode, "cache")) {
    xmlFreeDoc(doc);
    return false;
  }

  portalsNode = FindNodeByName(rootNode->children, (const xmlChar *) "portals");
  if (!portalsNode) {
    XBMC->Log(LOG_DEBUG, "%s: 'portals' element not found", __FUNCTION__);
  } else {
    xmlChar *num = NULL;
    bool found = false;
    for (node = portalsNode->children; node; node = node->next) {
      if (!xmlStrcmp(node->name, (const xmlChar *) "portal")) {
        num = xmlGetProp(node, (const xmlChar *) "num");
        if (num && !xmlStrcmp(num, (const xmlChar *) portalNum.c_str())) {
          portalNode = node;
          found = true;
        }
        xmlFree(num);
        if (found) break;
      }
    }
    if (portalNode) {
      std::string val;
      if (!m_bTokenManuallySet) {
        FindAndGetNodeValue(portalNode, (const xmlChar *) "token", val);
        SC_STR_SET(m_identity.token, val.c_str());

        XBMC->Log(LOG_DEBUG, "%s: token=%s", __FUNCTION__, m_identity.token);
      }
    }
  }

  xmlFreeDoc(doc);

  return true;
}

bool SData::SaveCache()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::string cacheFile;
  bool ret = false;
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL;
  xmlNodePtr node = NULL;
  xmlNodePtr portalsNode = NULL;
  xmlNodePtr portalNode = NULL;
  std::string portalNum = Utils::ToString(g_iActivePortal);

  cacheFile = Utils::GetFilePath("cache.xml");

  ret = Open(cacheFile, doc, rootNode, "cache");
  if (!ret) {
    if (!doc) {
      doc = xmlNewDoc((const xmlChar *) XML_DEFAULT_VERSION);
    }
    if (rootNode) {
      xmlUnlinkNode(rootNode);
      xmlFreeNode(rootNode);
    }
    rootNode = xmlNewDocNode(doc, NULL, (const xmlChar *) "cache", NULL);
    xmlDocSetRootElement(doc, rootNode);
  }

  portalsNode = FindNodeByName(rootNode->children, (const xmlChar *) "portals");
  if (!portalsNode) {
    portalsNode = xmlNewChild(rootNode, NULL, (const xmlChar *) "portals", NULL);
  }

  xmlChar *num = NULL;
  for (node = portalsNode->children; node; node = node->next) {
    if (!xmlStrcmp(node->name, (const xmlChar *) "portal")) {
      num = xmlGetProp(node, (const xmlChar *) "num");
      if (!num || !xmlStrlen(num) || portalNode) {
        xmlNodePtr tmp = node;
        node = tmp->prev;
        xmlUnlinkNode(tmp);
        xmlFreeNode(tmp);
      } else if (num && !xmlStrcmp(num, (const xmlChar *) portalNum.c_str())) {
        portalNode = node;
      }
      xmlFree(num);
    }
  }
  if (!portalNode) {
    portalNode = xmlNewChild(portalsNode, NULL, (const xmlChar *) "portal", NULL);
    xmlNewProp(portalNode, (const xmlChar *) "num", (const xmlChar *) portalNum.c_str());
  }

  if (!m_bTokenManuallySet) {
    FindAndSetNodeValue(portalNode, (const xmlChar *) "token", (const xmlChar *) m_identity.token);
  }

  ret = xmlSaveFormatFileEnc(cacheFile.c_str(), doc, xmlGetCharEncodingName(XML_CHAR_ENCODING_UTF8), 1) >= 0;
  if (!ret) {
    XBMC->Log(LOG_ERROR, "%s: failed to save cache file", __FUNCTION__);
  }

  xmlFreeDoc(doc);

  return ret;
}

SError SData::InitAPI()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_bInitedApi = false;

  m_api->SetIdentity(&m_identity);
  m_api->SetEndpoint(g_strServer);
  m_api->SetTimeout((unsigned int) g_iConnectionTimeout);

  m_bInitedApi = true;

  return SERROR_OK;
}

SError SData::DoHandshake()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value parsed;

  if (!m_api->STBHandshake(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: STBHandshake failed", __FUNCTION__);
    return SERROR_AUTHENTICATION;
  }

  if (parsed["js"].isMember("token"))
    SC_STR_SET(m_identity.token, parsed["js"]["token"].asCString());

  XBMC->Log(LOG_DEBUG, "%s: token=%s", __FUNCTION__, m_identity.token);

  if (parsed["js"].isMember("not_valid"))
    m_identity.valid_token = !Utils::GetIntFromJsonValue(parsed["js"]["not_valid"]);

  return SERROR_OK;
}

SError SData::DoAuth()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value parsed;
  SError ret(SERROR_OK);

  if (!m_api->STBDoAuth(parsed)) {
    XBMC->Log(LOG_ERROR, "%s: STBDoAuth failed", __FUNCTION__);
    return SERROR_AUTHENTICATION;
  }

  if (parsed.isMember("js") && !parsed["js"].asBool())
    ret = SERROR_AUTHENTICATION;

  return ret;
}

SError SData::LoadProfile(bool bAuthSecondStep)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Json::Value parsed;
  SError ret(SERROR_OK);

  if (!m_api->STBGetProfile(bAuthSecondStep, parsed)) {
    XBMC->Log(LOG_ERROR, "%s: STBGetProfile failed", __FUNCTION__);
    return SERROR_AUTHENTICATION;
  }

  sc_stb_profile_defaults(&m_profile);

  if (parsed["js"].isMember("store_auth_data_on_stb"))
    m_profile.store_auth_data_on_stb = Utils::GetBoolFromJsonValue(parsed["js"]["store_auth_data_on_stb"]);

  if (parsed["js"].isMember("status"))
    m_profile.status = Utils::GetIntFromJsonValue(parsed["js"]["status"]);

  SC_STR_SET(m_profile.msg, !parsed["js"].isMember("msg")
    ? ""
    : parsed["js"]["msg"].asCString());

  SC_STR_SET(m_profile.block_msg, !parsed["js"].isMember("block_msg")
    ? ""
    : parsed["js"]["block_msg"].asCString());

  if (parsed["js"].isMember("watchdog_timeout"))
    m_profile.watchdog_timeout = Utils::GetIntFromJsonValue(parsed["js"]["watchdog_timeout"]);

  if (parsed["js"].isMember("timeslot"))
    m_profile.timeslot = Utils::GetDoubleFromJsonValue(parsed["js"]["timeslot"]);

  XBMC->Log(LOG_DEBUG, "%s: timeslot=%f", __FUNCTION__, m_profile.timeslot);

  if (m_profile.store_auth_data_on_stb && !SaveCache())
    return SERROR_UNKNOWN;

  switch (m_profile.status) {
    case 0:
      break;
    case 2:
      ret = DoAuth();
      if (ret != SERROR_OK)
        return ret;

      return LoadProfile(true);
    case 1:
    default:
      m_strLastUnknownError = m_profile.msg;
      XBMC->Log(LOG_ERROR, "%s: status=%i | msg=%s | block_msg=%s",
        __FUNCTION__, m_profile.status, m_profile.msg, m_profile.block_msg);
      return SERROR_UNKNOWN;
  }

  return ret;
}

SError SData::Authenticate()
{
  SError ret(SERROR_OK);
  int iMaxRetires = 5;
  int iNumRetries = 0;

  m_bAuthenticated = false;

  while (!m_bAuthenticated && ++iNumRetries <= iMaxRetires) {
    // notify once after the first try failed
    if (iNumRetries == 2)
      QueueErrorNotification(SERROR_AUTHENTICATION);

    // don't sleep on first try
    if (iNumRetries > 1)
      usleep(5000000);

    if (!m_bTokenManuallySet && SERROR_OK != (ret = DoHandshake()))
      continue;

    if (SERROR_OK != (ret = LoadProfile()))
      continue;

    m_bAuthenticated = true;
  }

  return ret;
}

SError SData::ReAuthenticate(bool bAuthorizationLost)
{
  SError ret(SERROR_OK);

  m_authMutex.Lock();

  if (bAuthorizationLost)
    QueueErrorNotification(SERROR_AUTHORIZATION);

  ret = Authenticate();

  if (ret == SERROR_OK)
    XBMC->QueueNotification(QUEUE_INFO,
      XBMC->GetLocalizedString(MSG_RE_AUTHENTICATED));

  m_authMutex.Unlock();

  return ret;
}

bool SData::IsInitialized()
{
  return (m_bInitedApi && m_bAuthenticated);
}

SError SData::Initialize()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SError ret;

  if (!m_bInitedApi && SERROR_OK != (ret = InitAPI()))
    return ret;

  if (!m_bAuthenticated && SERROR_OK != (ret = Authenticate()))
    return ret;

  if (!m_watchdog) {
    m_watchdog = new CWatchdog((int)m_profile.timeslot, m_api);
    m_watchdog->SetData(this);
  }

  if (m_watchdog && !m_watchdog->IsRunning() && !m_watchdog->CreateThread())
    XBMC->Log(LOG_DEBUG, "%s: %s", __FUNCTION__, "failed to start Watchdog");

  m_channelManager->SetAPI(m_api);

  m_xmltv->SetUseCache(g_bGuideCache);
  m_xmltv->SetCacheFile(Utils::GetFilePath("epg_xmltv.xml"));
  m_xmltv->SetCacheExpiry((unsigned int) g_iGuideCacheHours * 3600);

  return SERROR_OK;
}

int SData::ParseEPG(Json::Value &parsed, time_t iStart, time_t iEnd, int iChannelNumber, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  time_t iStartTimestamp;
  time_t iStopTimestamp;
  int iEntriesTransfered(0);

  for (Json::Value::iterator it = parsed.begin(); it != parsed.end(); ++it) {
    iStartTimestamp = Utils::GetIntFromJsonValue((*it)["start_timestamp"]);
    iStopTimestamp = Utils::GetIntFromJsonValue((*it)["stop_timestamp"]);

    if (!(iStartTimestamp > iStart && iStopTimestamp < iEnd)) {
      continue;
    }

    EPG_TAG tag;
    memset(&tag, 0, sizeof(EPG_TAG));

    tag.iUniqueBroadcastId = Utils::GetIntFromJsonValue((*it)["id"]);
    tag.strTitle = (*it)["name"].asCString();
    tag.iChannelNumber = iChannelNumber;
    tag.startTime = iStartTimestamp;
    tag.endTime = iStopTimestamp;
    tag.strPlot = (*it)["descr"].asCString();
    tag.iFlags = EPG_TAG_FLAG_UNDEFINED;

    PVR->TransferEpgEntry(handle, &tag);
    iEntriesTransfered++;
  }

  return iEntriesTransfered;
}

int SData::ParseEPGXMLTV(int iChannelNumber, std::string &strChannelName, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::string strChanNum;
  XMLTV::Channel *chan = NULL;
  int iEntriesTransfered(0);

  strChanNum = Utils::ToString(iChannelNumber);

  chan = m_xmltv->GetChannelById(strChanNum);
  if (!chan)
    chan = m_xmltv->GetChannelByDisplayName(strChannelName);

  if (!chan) {
    XBMC->Log(LOG_DEBUG, "%s: channel \"%s\" not found", __FUNCTION__, strChanNum.c_str());
    return iEntriesTransfered;
  }

  for (std::vector<XMLTV::Programme>::iterator it = chan->programmes.begin(); it != chan->programmes.end(); ++it) {
    if (!(it->start > iStart && it->stop < iEnd))
      continue;

    EPG_TAG tag;
    memset(&tag, 0, sizeof(EPG_TAG));

    tag.iUniqueBroadcastId = it->extra.broadcastId;
    tag.strTitle = it->title.c_str();
    tag.iChannelNumber = iChannelNumber;
    tag.startTime = it->start;
    tag.endTime = it->stop;
    tag.strPlot = it->desc.c_str();
    tag.strCast = it->extra.cast.c_str();
    tag.strDirector = it->extra.directors.c_str();
    tag.strWriter = it->extra.writers.c_str();
    tag.iYear = Utils::StringToInt(it->date.substr(0, 4)); // year only
    tag.strIconPath = it->icon.c_str();
    tag.iGenreType = it->extra.genreType;
    if (tag.iGenreType == EPG_GENRE_USE_STRING)
      tag.strGenreDescription = it->extra.genreDescription.c_str();
    tag.firstAired = it->previouslyShown;
    tag.iStarRating = Utils::StringToInt(it->starRating.substr(0, 1)); // numerator only
    tag.iEpisodeNumber = it->episodeNumber;
    tag.strEpisodeName = it->subTitle.c_str();
    tag.iFlags = EPG_TAG_FLAG_UNDEFINED;

    PVR->TransferEpgEntry(handle, &tag);
    iEntriesTransfered++;
  }

  return iEntriesTransfered;
}

bool SData::LoadEPGForChannel(SC::Channel &channel, time_t iStart, time_t iEnd, ADDON_HANDLE handle)
{
  std::string strChannelId;
  int iEntriesTransfered(0);

  strChannelId = Utils::ToString(channel.channelId);

  switch (g_iGuidePreference) {
    case PREFER_PROVIDER:
    case PROVIDER_ONLY:
      if (!m_epgData.empty())
        iEntriesTransfered = ParseEPG(m_epgData["js"]["data"][strChannelId.c_str()], iStart, iEnd, channel.number, handle);

      if (g_iGuidePreference == PROVIDER_ONLY)
        break;

      if (iEntriesTransfered == 0 && m_xmltv)
        ParseEPGXMLTV(channel.number, channel.name, iStart, iEnd, handle);

      break;
    case PREFER_XMLTV:
    case XMLTV_ONLY:
      if (m_xmltv)
        iEntriesTransfered = ParseEPGXMLTV(channel.number, channel.name, iStart, iEnd, handle);

      if (g_iGuidePreference == XMLTV_ONLY)
        break;

      if (!m_epgData.empty() && iEntriesTransfered == 0)
        ParseEPG(m_epgData["js"]["data"][strChannelId.c_str()], iStart, iEnd, channel.number, handle);

      break;
  }

  return true;
}

SError SData::LoadEPG(time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  uint32_t iPeriod;
  HTTPSocket::Scope scope;
  std::string strXmltvPath;
  uint32_t iCacheExpiry;
  int iMaxRetires(5);
  int iNumRetries(0);
  bool bLoadedProvider(false);
  bool bLoadedXmltv(false);

  m_epgMutex.Lock();

  //TODO limit period to 24 hours for ITVGetEPGInfo. large amount of channels over too many days could exceed server max memory allowed for that request
  iPeriod = (iEnd - iStart) / 3600;

  if (g_iXmltvScope == REMOTE_URL) {
    scope = HTTPSocket::SCOPE_REMOTE;
    strXmltvPath = g_strXmltvUrl;
  } else {
    scope = HTTPSocket::SCOPE_LOCAL;
    strXmltvPath = g_strXmltvPath;
  }

  iCacheExpiry = g_iGuideCacheHours * 3600;

  //TODO merge with LoadData() when multiple PVR clients are properly supported
  //TODO replace with auth check
  if (g_iGuidePreference != XMLTV_ONLY && (IsInitialized() || SERROR_OK == Initialize())) {
    while (!bLoadedProvider && ++iNumRetries <= iMaxRetires) {
      // don't sleep on first try
      if (iNumRetries > 1)
        usleep(5000000);

      std::string cacheFile;
      if (g_bGuideCache)
        cacheFile = Utils::GetFilePath("epg_provider.json");

      if (!(bLoadedProvider = m_api->ITVGetEPGInfo(iPeriod, m_epgData, cacheFile, iCacheExpiry))) {
          XBMC->Log(LOG_ERROR, "%s: ITVGetEPGInfo failed", __FUNCTION__);
          if (g_bGuideCache && XBMC->FileExists(cacheFile.c_str(), false)) {
#ifdef TARGET_WINDOWS
              DeleteFile(cacheFile.c_str());
#else
              XBMC->DeleteFile(cacheFile.c_str());
#endif
          }
      }
    }
  }

  iNumRetries = 0;

  if (g_iGuidePreference != PROVIDER_ONLY && !strXmltvPath.empty() && m_xmltv) {
    while (!bLoadedXmltv && ++iNumRetries <= iMaxRetires) {
      // don't sleep on first try
      if (iNumRetries > 1)
        usleep(5000000);

      if (!(bLoadedXmltv = m_xmltv->Parse(scope, strXmltvPath)))
        XBMC->Log(LOG_ERROR, "%s: XMLTV Parse failed", __FUNCTION__);
    }
  }

  m_epgMutex.Unlock();

  return (bLoadedProvider || bLoadedXmltv) ? SERROR_OK : SERROR_LOAD_EPG;
}

void SData::UnloadEPG()
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  time_t now;

  m_epgMutex.Lock();

  time(&now);
  if ((m_iLastEpgAccessTime + 30 * 60) < now) {
    m_epgData.clear();

    if (m_xmltv)
      m_xmltv->Clear();
  }

  m_epgMutex.Unlock();
}

bool SData::LoadData(void)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  sc_identity_defaults(&m_identity);
  SC_STR_SET(m_identity.mac, g_strMac.c_str());
  SC_STR_SET(m_identity.time_zone, g_strTimeZone.c_str());
  SC_STR_SET(m_identity.token, g_strToken.c_str());
  SC_STR_SET(m_identity.login, g_strLogin.c_str());
  SC_STR_SET(m_identity.password, g_strPassword.c_str());
  SC_STR_SET(m_identity.serial_number, g_strSerialNumber.c_str());
  SC_STR_SET(m_identity.device_id, g_strDeviceId.c_str());
  SC_STR_SET(m_identity.device_id2, g_strDeviceId2.c_str());
  SC_STR_SET(m_identity.signature, g_strSignature.c_str());

  // skip handshake if token setting was set
  if (strlen(m_identity.token) > 0)
    m_bTokenManuallySet = true;

  LoadCache();

  return true;
}

PVR_ERROR SData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SC::Channel *thisChannel = NULL;
  time_t now;
  SError ret;

  thisChannel = m_channelManager->GetChannel(channel.iUniqueId);
  if (thisChannel == nullptr) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %d - %s",
    __FUNCTION__, iStart, iEnd, thisChannel->number, thisChannel->name.c_str());

  time(&now);
  m_iLastEpgAccessTime = now;
  if (m_iNextEpgLoadTime < now) {
    // limit to 1 hour if caching is disabled
    m_iNextEpgLoadTime = now + (g_bGuideCache ? g_iGuideCacheHours : 1) * 3600;
    XBMC->Log(LOG_DEBUG, "%s: m_iNextEpgLoadTime=%d", __FUNCTION__, m_iNextEpgLoadTime);

    ret = LoadEPG(iStart, iEnd);
    if (ret != SERROR_OK)
      QueueErrorNotification(ret);
  }

  if (!LoadEPGForChannel(*thisChannel, iStart, iEnd, handle))
    return PVR_ERROR_UNKNOWN;

  return PVR_ERROR_NO_ERROR;
}

int SData::GetChannelGroupsAmount(void)
{
  return m_channelManager->GetChannelGroups().size();
}

PVR_ERROR SData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SError ret;

  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  //TODO merge with LoadData() when multiple PVR clients are properly supported
  //TODO replace with auth check
  if (!IsInitialized() && SERROR_OK != (ret = Initialize())) {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  ret = m_channelManager->LoadChannelGroups();
  if (ret != SERROR_OK) {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::vector<SC::ChannelGroup> channelGroups;

  channelGroups = m_channelManager->GetChannelGroups();
  for (std::vector<SC::ChannelGroup>::iterator group = channelGroups.begin(); group != channelGroups.end(); ++group) {
    // exclude group id '*' (all)
    if (!group->id.compare("*"))
      continue;

    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(tag));

    strncpy(tag.strGroupName, group->name.c_str(), sizeof(tag.strGroupName) - 1);
    tag.bIsRadio = false;

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SC::ChannelGroup *channelGroup;

  channelGroup = m_channelManager->GetChannelGroup(group.strGroupName);
  if (channelGroup == nullptr) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::vector<SC::Channel> channels;

  channels = m_channelManager->GetChannels();
  for (std::vector<SC::Channel>::iterator channel = channels.begin(); channel != channels.end(); ++channel) {
    if (channel->tvGenreId.compare(channelGroup->id))
      continue;

    PVR_CHANNEL_GROUP_MEMBER tag;
    memset(&tag, 0, sizeof(tag));

    strncpy(tag.strGroupName, channelGroup->name.c_str(), sizeof(tag.strGroupName) - 1);
    tag.iChannelUniqueId = channel->uniqueId;
    tag.iChannelNumber = channel->number;

    PVR->TransferChannelGroupMember(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

int SData::GetChannelsAmount(void)
{
  return m_channelManager->GetChannels().size();
}

PVR_ERROR SData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  SError ret;
  std::vector<SC::Channel> channels;

  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  //TODO merge with LoadData() when multiple PVR clients are properly supported
  //TODO replace with auth check
  if (!IsInitialized() && SERROR_OK != (ret = Initialize())) {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  ret = m_channelManager->LoadChannels();
  if (ret != SERROR_OK) {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  channels = m_channelManager->GetChannels();
  for (std::vector<SC::Channel>::iterator channel = channels.begin(); channel != channels.end(); ++channel) {
    PVR_CHANNEL tag;
    memset(&tag, 0, sizeof(tag));

    tag.iUniqueId = channel->uniqueId;
    tag.bIsRadio = false;
    tag.iChannelNumber = channel->number;
    strncpy(tag.strChannelName, channel->name.c_str(), sizeof(tag.strChannelName) - 1);
    strncpy(tag.strStreamURL, channel->streamUrl.c_str(), sizeof(tag.strStreamURL) - 1);
    strncpy(tag.strIconPath, channel->iconPath.c_str(), sizeof(tag.strIconPath) - 1);
    tag.bIsHidden = false;

    PVR->TransferChannelEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

const char* SData::GetChannelStreamURL(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  //TODO merge with LoadData() when multiple PVR clients are properly supported
  //TODO replace with auth check
  if (!IsInitialized() && SERROR_OK != Initialize()) {
    return "";
  }

  SC::Channel *thisChannel = NULL;
  std::string strCmd;
  size_t pos;

  thisChannel = m_channelManager->GetChannel(channel.iUniqueId);
  if (thisChannel == nullptr) {
    XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
    return "";
  }

  if (thisChannel->cmd.find("matrix") != std::string::npos) {
    // non-standard call to server
    XBMC->Log(LOG_DEBUG, "%s: getting matrix stream url", __FUNCTION__);

    std::vector<std::string> strSplit;
    std::ostringstream oss;
    HTTPSocket::Request request;
    HTTPSocket::Response response;
    HTTPSocket sock(g_iConnectionTimeout);
    bool bFailed(false);

    strSplit = StringUtils::Split(thisChannel->cmd, "/");
    if (!strSplit.empty()) {
      oss << m_api->GetBasePath();
      oss << "server/api/matrix.php";
      oss << "?channel=" << Utils::UrlEncode(strSplit.back());
      oss << "&mac=" << Utils::UrlEncode(m_identity.mac);
      request.url = oss.str();

      if (sock.Execute(request, response)) {
        strSplit = StringUtils::Split(response.body, " ");
        if (!strSplit.empty()) {
          strCmd = strSplit.back();
        } else {
          XBMC->Log(LOG_ERROR, "%s: empty response?", __FUNCTION__);
          bFailed = true;
        }
      } else {
        XBMC->Log(LOG_ERROR, "%s: matrix call failed", __FUNCTION__);
        bFailed = true;
      }
    } else {
      XBMC->Log(LOG_ERROR, "%s: not a matrix channel?", __FUNCTION__);
      bFailed = true;
    }

    // fall back. maybe this is a valid, regular cmd/url
    if (bFailed) {
      XBMC->Log(LOG_DEBUG, "%s: falling back to original channel cmd", __FUNCTION__);
      strCmd = thisChannel->cmd;
    }

    // cmd format
    // (?:ffrt\d*\s|)(.*)
    if ((pos = strCmd.find(" ")) != std::string::npos)
      m_PlaybackURL = strCmd.substr(pos + 1);
    else
      m_PlaybackURL = strCmd;
  } else {
    m_PlaybackURL = m_channelManager->GetStreamURL(*thisChannel);
  }

  if (m_PlaybackURL.empty()) {
    XBMC->Log(LOG_ERROR, "%s: no stream url found", __FUNCTION__);
    QueueErrorNotification(SERROR_STREAM_URL);
  } else {
    // protocol options for http(s) urls only
    // <= zero disables timeout
    if (m_PlaybackURL.find("http") == 0 && g_iConnectionTimeout > 0)
      m_PlaybackURL += "|Connection-Timeout=" + Utils::ToString(g_iConnectionTimeout);

    XBMC->Log(LOG_DEBUG, "%s: stream url: %s", __FUNCTION__, m_PlaybackURL.c_str());
  }

  return m_PlaybackURL.c_str();
}
