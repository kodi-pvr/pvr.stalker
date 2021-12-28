/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SData.h"

#include "Utils.h"
#include "libstalkerclient/util.h"

#include <chrono>
#include <cmath>
#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

#define SERROR_MSG_UNKNOWN 30501
#define SERROR_MSG_INITIALIZE 30502
#define SERROR_MSG_API 30503
#define SERROR_MSG_AUTHENTICATION 30504
#define SERROR_MSG_LOAD_CHANNELS 30505
#define SERROR_MSG_LOAD_CHANNEL_GROUPS 30506
#define SERROR_MSG_LOAD_EPG 30507
#define SERROR_MSG_STREAM_URL 30508
#define SERROR_MSG_AUTHORIZATION 30509
#define MSG_RE_AUTHENTICATED 30510

SData::SData() : Base::Cache()
{
  sc_identity_defaults(&m_identity);
  sc_stb_profile_defaults(&m_profile);
}

SData::~SData()
{
  m_epgThreadActive = false;
  if (m_epgThread.joinable())
    m_epgThread.join();

  delete m_api;
  delete m_sessionManager;
  delete m_channelManager;
  delete m_guideManager;
}

void SData::QueueErrorNotification(SError error) const
{
  int errorMsg = 0;

  switch (error)
  {
    case SERROR_INITIALIZE:
      errorMsg = SERROR_MSG_INITIALIZE;
      break;
    case SERROR_API:
      errorMsg = SERROR_MSG_API;
      break;
    case SERROR_AUTHENTICATION:
      errorMsg = SERROR_MSG_AUTHENTICATION;
      break;
    case SERROR_LOAD_CHANNELS:
      errorMsg = SERROR_MSG_LOAD_CHANNELS;
      break;
    case SERROR_LOAD_CHANNEL_GROUPS:
      errorMsg = SERROR_MSG_LOAD_CHANNEL_GROUPS;
      break;
    case SERROR_LOAD_EPG:
      errorMsg = SERROR_MSG_LOAD_EPG;
      break;
    case SERROR_STREAM_URL:
      errorMsg = SERROR_MSG_STREAM_URL;
      break;
    case SERROR_AUTHORIZATION:
      errorMsg = SERROR_MSG_AUTHORIZATION;
      break;
    case SERROR_UNKNOWN:
    default:
      if (m_sessionManager->GetLastUnknownError().empty())
      {
        errorMsg = SERROR_MSG_UNKNOWN;
        break;
      }
      kodi::QueueNotification(QUEUE_ERROR, "", m_sessionManager->GetLastUnknownError());
      break;
  }

  if (errorMsg > 0)
    kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(errorMsg));
}

bool SData::LoadCache()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::string cacheFile;
  xmlDocPtr doc = nullptr;
  xmlNodePtr rootNode = nullptr;
  xmlNodePtr node = nullptr;
  xmlNodePtr portalsNode = nullptr;
  xmlNodePtr portalNode = nullptr;
  std::string portalNum = std::to_string(settings.activePortal);

  cacheFile = Utils::GetFilePath("cache.xml");

  if (!Open(cacheFile, doc, rootNode, "cache"))
  {
    xmlFreeDoc(doc);
    return false;
  }

  portalsNode = FindNodeByName(rootNode->children, (const xmlChar*)"portals");
  if (!portalsNode)
  {
    kodi::Log(ADDON_LOG_DEBUG, "%s: 'portals' element not found", __func__);
  }
  else
  {
    xmlChar* num = nullptr;
    bool found = false;
    for (node = portalsNode->children; node; node = node->next)
    {
      if (!xmlStrcmp(node->name, (const xmlChar*)"portal"))
      {
        num = xmlGetProp(node, (const xmlChar*)"num");
        if (num && !xmlStrcmp(num, (const xmlChar*)portalNum.c_str()))
        {
          portalNode = node;
          found = true;
        }
        xmlFree(num);
        if (found)
          break;
      }
    }
    if (portalNode)
    {
      std::string val;
      if (!m_tokenManuallySet)
      {
        FindAndGetNodeValue(portalNode, (const xmlChar*)"token", val);
        SC_STR_SET(m_identity.token, val.c_str());

        kodi::Log(ADDON_LOG_DEBUG, "%s: token=%s", __func__, m_identity.token);
      }
    }
  }

  xmlFreeDoc(doc);

  return true;
}

bool SData::SaveCache()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::string cacheFile;
  bool ret;
  xmlDocPtr doc = nullptr;
  xmlNodePtr rootNode = nullptr;
  xmlNodePtr node = nullptr;
  xmlNodePtr portalsNode = nullptr;
  xmlNodePtr portalNode = nullptr;
  std::string portalNum = std::to_string(settings.activePortal);

  cacheFile = Utils::GetFilePath("cache.xml");

  ret = Open(cacheFile, doc, rootNode, "cache");
  if (!ret)
  {
    if (!doc)
    {
      doc = xmlNewDoc((const xmlChar*)XML_DEFAULT_VERSION);
    }
    if (rootNode)
    {
      xmlUnlinkNode(rootNode);
      xmlFreeNode(rootNode);
    }
    rootNode = xmlNewDocNode(doc, nullptr, (const xmlChar*)"cache", nullptr);
    xmlDocSetRootElement(doc, rootNode);
  }

  portalsNode = FindNodeByName(rootNode->children, (const xmlChar*)"portals");
  if (!portalsNode)
  {
    portalsNode = xmlNewChild(rootNode, nullptr, (const xmlChar*)"portals", nullptr);
  }

  xmlChar* num = nullptr;
  for (node = portalsNode->children; node; node = node->next)
  {
    if (!xmlStrcmp(node->name, (const xmlChar*)"portal"))
    {
      num = xmlGetProp(node, (const xmlChar*)"num");
      if (!num || !xmlStrlen(num) || portalNode)
      {
        xmlNodePtr tmp = node;
        node = tmp->prev;
        xmlUnlinkNode(tmp);
        xmlFreeNode(tmp);
      }
      else if (num && !xmlStrcmp(num, (const xmlChar*)portalNum.c_str()))
      {
        portalNode = node;
      }
      xmlFree(num);
    }
  }
  if (!portalNode)
  {
    portalNode = xmlNewChild(portalsNode, nullptr, (const xmlChar*)"portal", nullptr);
    xmlNewProp(portalNode, (const xmlChar*)"num", (const xmlChar*)portalNum.c_str());
  }

  if (!m_tokenManuallySet)
  {
    FindAndSetNodeValue(portalNode, (const xmlChar*)"token", (const xmlChar*)m_identity.token);
  }

  ret = xmlSaveFormatFileEnc(cacheFile.c_str(), doc, xmlGetCharEncodingName(XML_CHAR_ENCODING_UTF8),
                             1) >= 0;
  if (!ret)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: failed to save cache file", __func__);
  }

  xmlFreeDoc(doc);

  return ret;
}

bool SData::ReloadSettings()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SError ret;

  sc_identity_defaults(&m_identity);
  SC_STR_SET(m_identity.mac, settings.mac.c_str());
  SC_STR_SET(m_identity.time_zone, settings.timeZone.c_str());
  SC_STR_SET(m_identity.token, settings.token.c_str());
  SC_STR_SET(m_identity.login, settings.login.c_str());
  SC_STR_SET(m_identity.password, settings.password.c_str());
  SC_STR_SET(m_identity.serial_number, settings.serialNumber.c_str());
  SC_STR_SET(m_identity.device_id, settings.deviceId.c_str());
  SC_STR_SET(m_identity.device_id2, settings.deviceId2.c_str());
  SC_STR_SET(m_identity.signature, settings.signature.c_str());

  // skip handshake if token setting was set
  if (strlen(m_identity.token) > 0)
    m_tokenManuallySet = true;

  LoadCache();

  m_api->SetIdentity(&m_identity);
  m_api->SetEndpoint(settings.server);
  m_api->SetTimeout(settings.connectionTimeout);

  m_sessionManager->SetIdentity(&m_identity, m_tokenManuallySet);
  m_sessionManager->SetProfile(&m_profile);
  m_sessionManager->SetAPI(m_api);
  m_sessionManager->SetStatusCallback([this](SError err) {
    if (err == SERROR_OK)
    {
      kodi::QueueNotification(QUEUE_INFO, "", kodi::addon::GetLocalizedString(MSG_RE_AUTHENTICATED));
    }
    else
    {
      QueueErrorNotification(err);
    }
  });

  m_channelManager->SetAPI(m_api);

  m_guideManager->SetAPI(m_api);
  m_guideManager->SetGuidePreference(settings.guidePreference);
  m_guideManager->SetCacheOptions(settings.guideCache, settings.guideCacheHours * 3600);

  ret = Authenticate();
  if (ret != SERROR_OK)
    QueueErrorNotification(ret);

  return ret == SERROR_OK;
}

bool SData::IsAuthenticated() const
{
  return m_sessionManager->IsAuthenticated();
}

SError SData::Authenticate()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SError ret;

  if (!m_sessionManager->IsAuthenticated() && SERROR_OK != (ret = m_sessionManager->Authenticate()))
    return ret;

  if (m_profile.store_auth_data_on_stb && !SaveCache())
    return SERROR_UNKNOWN;

  return SERROR_OK;
}

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::localtime(&time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", tm);

  return buffer;
}

} // unnamed namespace

#define PORTAL_SUFFIX_FORMAT "%s_%d"

#define GET_SETTING_STR2(setting, name, portal, store, def) \
  sprintf(setting, PORTAL_SUFFIX_FORMAT, name, portal); \
  store = kodi::addon::GetSettingString(setting, def);

#define GET_SETTING_INT2(setting, name, portal, store, def) \
  sprintf(setting, PORTAL_SUFFIX_FORMAT, name, portal); \
  store = kodi::addon::GetSettingInt(setting, def);

ADDON_STATUS SData::Create()
{
  settings.activePortal = kodi::addon::GetSettingInt("active_portal", SC_SETTINGS_DEFAULT_ACTIVE_PORTAL);
  settings.connectionTimeout =
      kodi::addon::GetSettingInt("connection_timeout", SC_SETTINGS_DEFAULT_CONNECTION_TIMEOUT);
  // calc based on index (5 second steps)
  settings.connectionTimeout *= 5;

  char setting[256];
  int enumTemp;
  int portal = settings.activePortal;
  GET_SETTING_STR2(setting, "mac", portal, settings.mac, SC_SETTINGS_DEFAULT_MAC);
  GET_SETTING_STR2(setting, "server", portal, settings.server, SC_SETTINGS_DEFAULT_SERVER);
  GET_SETTING_STR2(setting, "time_zone", portal, settings.timeZone, SC_SETTINGS_DEFAULT_TIME_ZONE);
  GET_SETTING_STR2(setting, "login", portal, settings.login, SC_SETTINGS_DEFAULT_LOGIN);
  GET_SETTING_STR2(setting, "password", portal, settings.password, SC_SETTINGS_DEFAULT_PASSWORD);
  GET_SETTING_INT2(setting, "guide_preference", portal, enumTemp,
                   SC_SETTINGS_DEFAULT_GUIDE_PREFERENCE);
  settings.guidePreference = (SC::Settings::GuidePreference)enumTemp;
  GET_SETTING_INT2(setting, "guide_cache", portal, settings.guideCache,
                   SC_SETTINGS_DEFAULT_GUIDE_CACHE);
  GET_SETTING_INT2(setting, "guide_cache_hours", portal, settings.guideCacheHours,
                   SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS);
  GET_SETTING_INT2(setting, "xmltv_scope", portal, enumTemp, SC_SETTINGS_DEFAULT_XMLTV_SCOPE);
  settings.xmltvScope = (HTTPSocket::Scope)enumTemp;
  if (settings.xmltvScope == HTTPSocket::Scope::SCOPE_REMOTE)
  {
    GET_SETTING_STR2(setting, "xmltv_url", portal, settings.xmltvPath,
                     SC_SETTINGS_DEFAULT_XMLTV_URL);
  }
  else
  {
    GET_SETTING_STR2(setting, "xmltv_path", portal, settings.xmltvPath,
                     SC_SETTINGS_DEFAULT_XMLTV_PATH);
  }
  GET_SETTING_STR2(setting, "token", portal, settings.token, SC_SETTINGS_DEFAULT_TOKEN);
  GET_SETTING_STR2(setting, "serial_number", portal, settings.serialNumber,
                   SC_SETTINGS_DEFAULT_SERIAL_NUMBER);
  GET_SETTING_STR2(setting, "device_id", portal, settings.deviceId, SC_SETTINGS_DEFAULT_DEVICE_ID);
  GET_SETTING_STR2(setting, "device_id2", portal, settings.deviceId2,
                   SC_SETTINGS_DEFAULT_DEVICE_ID2);
  GET_SETTING_STR2(setting, "signature", portal, settings.signature, SC_SETTINGS_DEFAULT_SIGNATURE);

  kodi::Log(ADDON_LOG_DEBUG, "active_portal=%d", settings.activePortal);
  kodi::Log(ADDON_LOG_DEBUG, "connection_timeout=%d", settings.connectionTimeout);

  kodi::Log(ADDON_LOG_DEBUG, "mac=%s", settings.mac.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "server=%s", settings.server.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "timeZone=%s", settings.timeZone.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "login=%s", settings.login.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "password=%s", settings.password.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "guidePreference=%d", settings.guidePreference);
  kodi::Log(ADDON_LOG_DEBUG, "guideCache=%d", settings.guideCache);
  kodi::Log(ADDON_LOG_DEBUG, "guideCacheHours=%d", settings.guideCacheHours);
  kodi::Log(ADDON_LOG_DEBUG, "xmltvScope=%d", settings.xmltvScope);
  kodi::Log(ADDON_LOG_DEBUG, "xmltvPath=%s", settings.xmltvPath.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "token=%s", settings.token.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "serialNumber=%s", settings.serialNumber.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "deviceId=%s", settings.deviceId.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "deviceId2=%s", settings.deviceId2.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "signature=%s", settings.signature.c_str());

  if (!ReloadSettings())
  {
    return ADDON_STATUS_LOST_CONNECTION;
  }

  return ADDON_STATUS_OK;
}

PVR_ERROR SData::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsRecordings(false);
  capabilities.SetSupportsRecordingsRename(false);
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetBackendName(std::string& name)
{
  name = "Stalker Middleware";
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetBackendVersion(std::string& version)
{
  version = "Unknown";
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetConnectionString(std::string& connection)
{
  connection = settings.server;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetEPGForChannel(int channelUid,
                                  time_t start,
                                  time_t end,
                                  kodi::addon::PVREPGTagsResultSet& results)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SC::Channel* chan;
  time_t now;
  SError ret;

  chan = m_channelManager->GetChannel(channelUid);
  if (chan == nullptr)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: channel not found", __func__);
    return PVR_ERROR_SERVER_ERROR;
  }

  kodi::Log(ADDON_LOG_DEBUG, "%s: time range: %d - %d | %d - %s", __func__, start, end,
            chan->number, chan->name.c_str());

  m_epgMutex.lock();

  time(&now);
  m_lastEpgAccessTime = now;
  if (m_nextEpgLoadTime < now)
  {
    // limit to 1 hour if caching is disabled
    m_nextEpgLoadTime = now + (settings.guideCache ? settings.guideCacheHours : 1) * 3600;
    kodi::Log(ADDON_LOG_DEBUG, "%s: m_nextEpgLoadTime=%d", __func__, m_nextEpgLoadTime);

    if (IsAuthenticated())
    {
      ret = m_guideManager->LoadGuide(start, end);
      if (ret != SERROR_OK)
        QueueErrorNotification(ret);
    }

    ret = m_guideManager->LoadXMLTV(settings.xmltvScope, settings.xmltvPath);
    if (ret != SERROR_OK)
      QueueErrorNotification(ret);
  }

  std::vector<SC::Event> events;

  events = m_guideManager->GetChannelEvents(*chan, start, end);
  for (std::vector<SC::Event>::iterator event = events.begin(); event != events.end(); ++event)
  {
    kodi::addon::PVREPGTag tag;

    tag.SetUniqueBroadcastId(event->uniqueBroadcastId);
    tag.SetTitle(event->title);
    tag.SetUniqueChannelId(chan->uniqueId);
    tag.SetStartTime(event->startTime);
    tag.SetEndTime(event->endTime);
    tag.SetPlot(event->plot);
    tag.SetCast(event->cast);
    tag.SetDirector(event->directors);
    tag.SetWriter(event->writers);
    tag.SetYear(event->year);
    tag.SetIconPath(event->iconPath);
    tag.SetGenreType(event->genreType);
    if (tag.GetGenreType() == EPG_GENRE_USE_STRING)
      tag.SetGenreDescription(event->genreDescription);
    std::string strFirstAired(event->firstAired > 0 ? ParseAsW3CDateString(event->firstAired) : "");
    tag.SetFirstAired(strFirstAired);
    tag.SetStarRating(event->starRating);
    tag.SetSeriesNumber(EPG_TAG_INVALID_SERIES_EPISODE);
    tag.SetEpisodeNumber(event->episodeNumber);
    tag.SetEpisodePartNumber(EPG_TAG_INVALID_SERIES_EPISODE);
    tag.SetEpisodeName(event->episodeName);
    tag.SetFlags(EPG_TAG_FLAG_UNDEFINED);

    results.Add(tag);
  }

  m_epgMutex.unlock();

  if (!m_epgThread.joinable())
  {
    m_epgThreadActive = true;
    m_epgThread = std::thread([this] {
      unsigned int target(30000);
      unsigned int count;

      while (m_epgThreadActive)
      {
        kodi::Log(ADDON_LOG_DEBUG, "epgThread");

        time_t now;

        m_epgMutex.lock();

        time(&now);
        if ((m_lastEpgAccessTime + 30 * 60) < now)
          m_guideManager->Clear();

        m_epgMutex.unlock();

        count = 0;
        while (count < target)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          if (!m_epgThreadActive)
            break;
          count += 100;
        }
      }
    });
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroupsAmount(int& amount)
{
  amount = m_channelManager->GetChannelGroups().size();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SError ret;

  if (radio)
    return PVR_ERROR_NO_ERROR;

  if (!IsAuthenticated())
    return PVR_ERROR_SERVER_ERROR;

  ret = m_channelManager->LoadChannelGroups();
  if (ret != SERROR_OK)
  {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::vector<SC::ChannelGroup> channelGroups;

  channelGroups = m_channelManager->GetChannelGroups();
  for (std::vector<SC::ChannelGroup>::iterator group = channelGroups.begin();
       group != channelGroups.end(); ++group)
  {
    // exclude group id '*' (all)
    if (!group->id.compare("*"))
      continue;

    kodi::addon::PVRChannelGroup tag;

    tag.SetGroupName(group->name);
    tag.SetIsRadio(false);

    results.Add(tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                        kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SC::ChannelGroup* channelGroup;

  channelGroup = m_channelManager->GetChannelGroup(group.GetGroupName());
  if (channelGroup == nullptr)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: channel not found", __func__);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::vector<SC::Channel> channels;

  channels = m_channelManager->GetChannels();
  for (std::vector<SC::Channel>::iterator channel = channels.begin(); channel != channels.end();
       ++channel)
  {
    if (channel->tvGenreId.compare(channelGroup->id))
      continue;

    kodi::addon::PVRChannelGroupMember tag;

    tag.SetGroupName(channelGroup->name);
    tag.SetChannelUniqueId(channel->uniqueId);
    tag.SetChannelNumber(channel->number);

    results.Add(tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelsAmount(int& amount)
{
  amount = m_channelManager->GetChannels().size();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  SError ret;

  if (radio)
    return PVR_ERROR_NO_ERROR;

  if (!IsAuthenticated())
    return PVR_ERROR_SERVER_ERROR;

  ret = m_channelManager->LoadChannels();
  if (ret != SERROR_OK)
  {
    QueueErrorNotification(ret);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::vector<SC::Channel> channels;

  channels = m_channelManager->GetChannels();
  for (std::vector<SC::Channel>::iterator channel = channels.begin(); channel != channels.end();
       ++channel)
  {
    kodi::addon::PVRChannel tag;

    tag.SetUniqueId(channel->uniqueId);
    tag.SetIsRadio(false);
    tag.SetChannelNumber(channel->number);
    tag.SetChannelName(channel->name);
    tag.SetIconPath(channel->iconPath);
    tag.SetIsHidden(false);

    results.Add(tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR SData::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
                                            std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  const std::string strUrl = GetChannelStreamURL(channel);

  if (strUrl.empty())
    return PVR_ERROR_FAILED;

  properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, strUrl.c_str());
  properties.emplace_back(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");

  return PVR_ERROR_NO_ERROR;
}

std::string SData::GetChannelStreamURL(const kodi::addon::PVRChannel& channel) const
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  std::string streamUrl;

  if (!IsAuthenticated())
    return streamUrl;

  SC::Channel* chan;
  std::string cmd;
  size_t pos;

  chan = m_channelManager->GetChannel(channel.GetUniqueId());
  if (chan == nullptr)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: channel not found", __func__);
    return streamUrl;
  }

  kodi::Log(ADDON_LOG_DEBUG, "%s: cmd=%s", __func__, chan->cmd.c_str());

  if (chan->cmd.find("matrix") != std::string::npos)
  {
    // non-standard call to server
    kodi::Log(ADDON_LOG_DEBUG, "%s: getting matrix stream url", __func__);

    std::vector<std::string> strSplit;
    std::ostringstream oss;
    HTTPSocket::Request request;
    HTTPSocket::Response response;
    HTTPSocket sock(settings.connectionTimeout);
    bool failed(false);

    strSplit = kodi::tools::StringUtils::Split(chan->cmd, "/");
    if (!strSplit.empty())
    {
      oss << m_api->GetBasePath();
      oss << "server/api/matrix.php";
      oss << "?channel=" << Utils::UrlEncode(strSplit.back());
      oss << "&mac=" << Utils::UrlEncode(m_identity.mac);
      request.url = oss.str();

      if (sock.Execute(request, response))
      {
        strSplit = kodi::tools::StringUtils::Split(response.body, " ");
        if (!strSplit.empty())
        {
          cmd = strSplit.back();
        }
        else
        {
          kodi::Log(ADDON_LOG_ERROR, "%s: empty response?", __func__);
          failed = true;
        }
      }
      else
      {
        kodi::Log(ADDON_LOG_ERROR, "%s: matrix call failed", __func__);
        failed = true;
      }
    }
    else
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: not a matrix channel?", __func__);
      failed = true;
    }

    // fall back. maybe this is a valid, regular cmd/url
    if (failed)
    {
      kodi::Log(ADDON_LOG_DEBUG, "%s: falling back to original channel cmd", __func__);
      cmd = chan->cmd;
    }

    // cmd format
    // (?:ffrt\d*\s|)(.*)
    if ((pos = cmd.find(" ")) != std::string::npos)
      streamUrl = cmd.substr(pos + 1);
    else
      streamUrl = cmd;
  }
  else
  {
    streamUrl = m_channelManager->GetStreamURL(*chan);
  }

  if (streamUrl.empty())
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: no stream url found", __func__);
    QueueErrorNotification(SERROR_STREAM_URL);
  }
  else
  {
    // protocol options for http(s) urls only
    // <= zero disables timeout
    //        if (streamUrl.find("http") == 0 && settings.connectionTimeout > 0)
    //            streamUrl += "|Connection-Timeout=" + std::to_string(settings.connectionTimeout);

    kodi::Log(ADDON_LOG_DEBUG, "%s: streamUrl=%s", __func__, streamUrl.c_str());
  }

  return streamUrl;
}

ADDONCREATOR(SData)
