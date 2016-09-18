/*
 *      Copyright (C) 2015, 2016  Jamal Edey
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

#include <p8-platform/util/StringUtils.h>
#include <p8-platform/util/timeutils.h>
#include <p8-platform/util/util.h>

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

SData::SData() : Base::Cache() {
    m_tokenManuallySet = false;
    m_lastEpgAccessTime = 0;
    m_nextEpgLoadTime = 0;
    m_epgThreadActive = false;
    m_api = new SC::SAPI;
    m_sessionManager = new SC::SessionManager;
    m_channelManager = new SC::ChannelManager;
    m_guideManager = new SC::GuideManager;

    sc_identity_defaults(&m_identity);
    sc_stb_profile_defaults(&m_profile);
}

SData::~SData() {
    m_epgThreadActive = false;
    if (m_epgThread.joinable())
        m_epgThread.join();

    SAFE_DELETE(m_api);
    SAFE_DELETE(m_sessionManager);
    SAFE_DELETE(m_channelManager);
    SAFE_DELETE(m_guideManager);
}

void SData::QueueErrorNotification(SError error) {
    int errorMsg = 0;

    switch (error) {
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
            if (m_sessionManager->GetLastUnknownError().empty()) {
                errorMsg = SERROR_MSG_UNKNOWN;
                break;
            }
            XBMC->QueueNotification(QUEUE_ERROR, m_sessionManager->GetLastUnknownError().c_str());
            break;
    }

    if (errorMsg > 0)
        XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(errorMsg));
}

bool SData::LoadCache() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    std::string cacheFile;
    xmlDocPtr doc = NULL;
    xmlNodePtr rootNode = NULL;
    xmlNodePtr node = NULL;
    xmlNodePtr portalsNode = NULL;
    xmlNodePtr portalNode = NULL;
    std::string portalNum = Utils::ToString(settings.activePortal);

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
            if (!m_tokenManuallySet) {
                FindAndGetNodeValue(portalNode, (const xmlChar *) "token", val);
                SC_STR_SET(m_identity.token, val.c_str());

                XBMC->Log(LOG_DEBUG, "%s: token=%s", __FUNCTION__, m_identity.token);
            }
        }
    }

    xmlFreeDoc(doc);

    return true;
}

bool SData::SaveCache() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    std::string cacheFile;
    bool ret;
    xmlDocPtr doc = NULL;
    xmlNodePtr rootNode = NULL;
    xmlNodePtr node = NULL;
    xmlNodePtr portalsNode = NULL;
    xmlNodePtr portalNode = NULL;
    std::string portalNum = Utils::ToString(settings.activePortal);

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

    if (!m_tokenManuallySet) {
        FindAndSetNodeValue(portalNode, (const xmlChar *) "token", (const xmlChar *) m_identity.token);
    }

    ret = xmlSaveFormatFileEnc(cacheFile.c_str(), doc, xmlGetCharEncodingName(XML_CHAR_ENCODING_UTF8), 1) >= 0;
    if (!ret) {
        XBMC->Log(LOG_ERROR, "%s: failed to save cache file", __FUNCTION__);
    }

    xmlFreeDoc(doc);

    return ret;
}

bool SData::ReloadSettings() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

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
        if (err == SERROR_OK) {
            XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(MSG_RE_AUTHENTICATED));
        } else {
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

bool SData::IsAuthenticated() {
    return m_sessionManager->IsAuthenticated();
}

SError SData::Authenticate() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    SError ret;

    if (!m_sessionManager->IsAuthenticated() && SERROR_OK != (ret = m_sessionManager->Authenticate()))
        return ret;

    if (m_profile.store_auth_data_on_stb && !SaveCache())
        return SERROR_UNKNOWN;

    return SERROR_OK;
}

PVR_ERROR SData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    SC::Channel *chan;
    time_t now;
    SError ret;

    chan = m_channelManager->GetChannel(channel.iUniqueId);
    if (chan == nullptr) {
        XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
        return PVR_ERROR_SERVER_ERROR;
    }

    XBMC->Log(LOG_DEBUG, "%s: time range: %d - %d | %d - %s",
              __FUNCTION__, start, end, chan->number, chan->name.c_str());

    m_epgMutex.Lock();

    time(&now);
    m_lastEpgAccessTime = now;
    if (m_nextEpgLoadTime < now) {
        // limit to 1 hour if caching is disabled
        m_nextEpgLoadTime = now + (settings.guideCache ? settings.guideCacheHours : 1) * 3600;
        XBMC->Log(LOG_DEBUG, "%s: m_nextEpgLoadTime=%d", __FUNCTION__, m_nextEpgLoadTime);

        if (IsAuthenticated()) {
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
    for (std::vector<SC::Event>::iterator event = events.begin(); event != events.end(); ++event) {
        EPG_TAG tag;
        memset(&tag, 0, sizeof(EPG_TAG));

        tag.iUniqueBroadcastId = event->uniqueBroadcastId;
        tag.strTitle = event->title.c_str();
        tag.iChannelNumber = event->channelNumber;
        tag.startTime = event->startTime;
        tag.endTime = event->endTime;
        tag.strPlot = event->plot.c_str();
        tag.strCast = event->cast.c_str();
        tag.strDirector = event->directors.c_str();
        tag.strWriter = event->writers.c_str();
        tag.iYear = event->year;
        tag.strIconPath = event->iconPath.c_str();
        tag.iGenreType = event->genreType;
        if (tag.iGenreType == EPG_GENRE_USE_STRING)
            tag.strGenreDescription = event->genreDescription.c_str();
        tag.firstAired = event->firstAired;
        tag.iStarRating = event->starRating;
        tag.iEpisodeNumber = event->episodeNumber;
        tag.strEpisodeName = event->episodeName.c_str();
        tag.iFlags = EPG_TAG_FLAG_UNDEFINED;

        PVR->TransferEpgEntry(handle, &tag);
    }

    m_epgMutex.Unlock();

    if (!m_epgThread.joinable()) {
        m_epgThreadActive = true;
        m_epgThread = std::thread([this] {
            unsigned int target(30000);
            unsigned int count;

            while (m_epgThreadActive) {
                XBMC->Log(LOG_DEBUG, "epgThread");

                time_t now;

                m_epgMutex.Lock();

                time(&now);
                if ((m_lastEpgAccessTime + 30 * 60) < now)
                    m_guideManager->Clear();

                m_epgMutex.Unlock();

                count = 0;
                while (count < target) {
                    usleep(100000);
                    if (!m_epgThreadActive)
                        break;
                    count += 100;
                }
            }
        });
    }

    return PVR_ERROR_NO_ERROR;
}

int SData::GetChannelGroupsAmount() {
    return m_channelManager->GetChannelGroups().size();
}

PVR_ERROR SData::GetChannelGroups(ADDON_HANDLE handle, bool radio) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    SError ret;

    if (radio)
        return PVR_ERROR_NO_ERROR;

    if (!IsAuthenticated())
        return PVR_ERROR_SERVER_ERROR;

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

PVR_ERROR SData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group) {
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

int SData::GetChannelsAmount() {
    return m_channelManager->GetChannels().size();
}

PVR_ERROR SData::GetChannels(ADDON_HANDLE handle, bool radio) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    SError ret;

    if (radio)
        return PVR_ERROR_NO_ERROR;

    if (!IsAuthenticated())
        return PVR_ERROR_SERVER_ERROR;

    ret = m_channelManager->LoadChannels();
    if (ret != SERROR_OK) {
        QueueErrorNotification(ret);
        return PVR_ERROR_SERVER_ERROR;
    }

    std::vector<SC::Channel> channels;

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

const char *SData::GetChannelStreamURL(const PVR_CHANNEL &channel) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    if (!IsAuthenticated())
        return "";

    SC::Channel *chan;
    std::string cmd;
    size_t pos;

    chan = m_channelManager->GetChannel(channel.iUniqueId);
    if (chan == nullptr) {
        XBMC->Log(LOG_ERROR, "%s: channel not found", __FUNCTION__);
        return "";
    }

    XBMC->Log(LOG_DEBUG, "%s: cmd=%s", __FUNCTION__, chan->cmd.c_str());

    if (chan->cmd.find("matrix") != std::string::npos) {
        // non-standard call to server
        XBMC->Log(LOG_DEBUG, "%s: getting matrix stream url", __FUNCTION__);

        std::vector<std::string> strSplit;
        std::ostringstream oss;
        HTTPSocket::Request request;
        HTTPSocket::Response response;
        HTTPSocket sock(settings.connectionTimeout);
        bool failed(false);

        strSplit = StringUtils::Split(chan->cmd, "/");
        if (!strSplit.empty()) {
            oss << m_api->GetBasePath();
            oss << "server/api/matrix.php";
            oss << "?channel=" << Utils::UrlEncode(strSplit.back());
            oss << "&mac=" << Utils::UrlEncode(m_identity.mac);
            request.url = oss.str();

            if (sock.Execute(request, response)) {
                strSplit = StringUtils::Split(response.body, " ");
                if (!strSplit.empty()) {
                    cmd = strSplit.back();
                } else {
                    XBMC->Log(LOG_ERROR, "%s: empty response?", __FUNCTION__);
                    failed = true;
                }
            } else {
                XBMC->Log(LOG_ERROR, "%s: matrix call failed", __FUNCTION__);
                failed = true;
            }
        } else {
            XBMC->Log(LOG_ERROR, "%s: not a matrix channel?", __FUNCTION__);
            failed = true;
        }

        // fall back. maybe this is a valid, regular cmd/url
        if (failed) {
            XBMC->Log(LOG_DEBUG, "%s: falling back to original channel cmd", __FUNCTION__);
            cmd = chan->cmd;
        }

        // cmd format
        // (?:ffrt\d*\s|)(.*)
        if ((pos = cmd.find(" ")) != std::string::npos)
            m_currentPlaybackUrl = cmd.substr(pos + 1);
        else
            m_currentPlaybackUrl = cmd;
    } else {
        m_currentPlaybackUrl = m_channelManager->GetStreamURL(*chan);
    }

    if (m_currentPlaybackUrl.empty()) {
        XBMC->Log(LOG_ERROR, "%s: no stream url found", __FUNCTION__);
        QueueErrorNotification(SERROR_STREAM_URL);
    } else {
        // protocol options for http(s) urls only
        // <= zero disables timeout
//        if (m_currentPlaybackUrl.find("http") == 0 && settings.connectionTimeout > 0)
//            m_currentPlaybackUrl += "|Connection-Timeout=" + Utils::ToString(settings.connectionTimeout);

        XBMC->Log(LOG_DEBUG, "%s: m_currentPlaybackUrl=%s", __FUNCTION__, m_currentPlaybackUrl.c_str());
    }

    return m_currentPlaybackUrl.c_str();
}
