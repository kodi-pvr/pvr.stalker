/*
 *      Copyright (C) 2016  Jamal Edey
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

#include "GuideManager.h"

#if defined(_WIN32) || defined(_WIN64)
#define usleep(usec) Sleep((DWORD)(usec)/1000)
#else
#include <unistd.h>
#endif

#include "Utils.h"

using namespace ADDON;
using namespace SC;

GuideManager::GuideManager() : Base::GuideManager<Event>() {
    m_api = nullptr;
    m_guidePreference = (SC::Settings::GuidePreference) SC_SETTINGS_DEFAULT_GUIDE_PREFERENCE;
    m_useCache = SC_SETTINGS_DEFAULT_GUIDE_CACHE;
    m_expiry = SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS * 3600;
    m_xmltv = std::make_shared<XMLTV>();
}

GuideManager::~GuideManager() {
    m_api = nullptr;
    Clear();
}

SError GuideManager::LoadGuide(time_t start, time_t end) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_XMLTV_ONLY)
        return SERROR_OK;

    bool ret(false);
    int maxRetires(5);
    int numRetries(0);
    int period;
    std::string cacheFile;
    unsigned int cacheExpiry(0);

    //TODO limit period to 24 hours for ITVGetEPGInfo. large amount of channels over too many days could exceed server max memory allowed for that request
    period = (int) (end - start) / 3600;

    if (m_useCache) {
        cacheFile = Utils::GetFilePath("epg_provider.json");
        cacheExpiry = m_expiry;
    }

    while (!ret && ++numRetries <= maxRetires) {
        // don't sleep on first try
        if (numRetries > 1)
            usleep(5000000);

        if (!(ret = m_api->ITVGetEPGInfo(period, m_epgData, cacheFile, cacheExpiry))) {
            XBMC->Log(LOG_ERROR, "%s: ITVGetEPGInfo failed", __FUNCTION__);
            if (m_useCache && XBMC->FileExists(cacheFile.c_str(), false)) {
#ifdef TARGET_WINDOWS
                DeleteFile(cacheFile.c_str());
#else
                XBMC->DeleteFile(cacheFile.c_str());
#endif
            }
        }
    }

    return ret ? SERROR_OK : SERROR_LOAD_EPG;
}

SError GuideManager::LoadXMLTV(HTTPSocket::Scope scope, const std::string &path) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PROVIDER_ONLY || path.empty())
        return SERROR_OK;

    bool ret(false);
    int maxRetires(5);
    int numRetries(0);

    m_xmltv->SetUseCache(m_useCache);
    m_xmltv->SetCacheFile(Utils::GetFilePath("epg_xmltv.xml"));
    m_xmltv->SetCacheExpiry(m_expiry);

    while (!ret && ++numRetries <= maxRetires) {
        // don't sleep on first try
        if (numRetries > 1)
            usleep(5000000);

        if (!(ret = m_xmltv->Parse(scope, path)))
            XBMC->Log(LOG_ERROR, "%s: XMLTV Parse failed", __FUNCTION__);
    }

    return ret ? SERROR_OK : SERROR_LOAD_EPG;
}

int GuideManager::AddEvents(int type, std::vector<Event> &events, Channel &channel, time_t start, time_t end) {
    int addedEvents(0);

    if (type == 0) {
        try {
            std::string channelId;

            channelId = Utils::ToString(channel.channelId);
            if (!m_epgData.isMember("js") || !m_epgData["js"].isObject() || !m_epgData["js"].isMember("data") ||
                !m_epgData["js"]["data"].isMember(channelId)) {
                return addedEvents;
            }

            Json::Value value;
            time_t startTimestamp;
            time_t stopTimestamp;

            value = m_epgData["js"]["data"][channelId];
            if (!value.isObject() && !value.isArray())
                return addedEvents;

            for (Json::Value::iterator it = value.begin(); it != value.end(); ++it) {
                try {
                    startTimestamp = Utils::GetIntFromJsonValue((*it)["start_timestamp"]);
                    stopTimestamp = Utils::GetIntFromJsonValue((*it)["stop_timestamp"]);

                    if (start != 0 && end != 0 && !(startTimestamp >= start && stopTimestamp <= end))
                        continue;

                    Event e;
                    e.uniqueBroadcastId = Utils::GetIntFromJsonValue((*it)["id"]);
                    e.title = (*it)["name"].asCString();
                    e.channelNumber = channel.number;
                    e.startTime = startTimestamp;
                    e.endTime = stopTimestamp;
                    e.plot = (*it)["descr"].asCString();

                    events.push_back(e);
                    addedEvents++;
                } catch (const std::exception &ex) {
                    XBMC->Log(LOG_DEBUG, "%s: epg event excep. what=%s", __FUNCTION__, ex.what());
                }
            }
        } catch (const std::exception &ex) {
            XBMC->Log(LOG_ERROR, "%s: epg data excep. what=%s", __FUNCTION__, ex.what());
        }
    }

    if (type == 1) {
        std::string channelNum;
        XMLTV::Channel *c;

        channelNum = Utils::ToString(channel.number);
        c = m_xmltv->GetChannelById(channelNum);
        if (c == nullptr) {
            c = m_xmltv->GetChannelByDisplayName(channel.name);
        }
        if (c != nullptr) {
            for (std::vector<XMLTV::Programme>::iterator it = c->programmes.begin(); it != c->programmes.end(); ++it) {
                XMLTV::Programme *p = &(*it);

                if (start != 0 && end != 0 && !(p->start >= start && p->stop <= end))
                    continue;

                Event e;
                e.uniqueBroadcastId = p->extra.broadcastId;
                e.title = p->title;
                e.channelNumber = channel.number;
                e.startTime = p->start;
                e.endTime = p->stop;
                e.plot = p->desc;
                e.cast = p->extra.cast;
                e.directors = p->extra.directors;
                e.writers = p->extra.writers;
                e.year = Utils::StringToInt(p->date.substr(0, 4));
                e.iconPath = p->icon;
                e.genreType = p->extra.genreType;
                e.genreDescription = p->extra.genreDescription;
                e.firstAired = p->previouslyShown;
                e.starRating = Utils::StringToInt(p->starRating.substr(0, 1));
                e.episodeNumber = p->episodeNumber;
                e.episodeName = p->subTitle;

                events.push_back(e);
                addedEvents++;
            }
        }
    }

    return addedEvents;
}

std::vector<Event> GuideManager::GetChannelEvents(Channel &channel, time_t start, time_t end) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    std::vector<Event> events;
    int addedEvents;

    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_PROVIDER ||
        m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PROVIDER_ONLY) {
        addedEvents = AddEvents(0, events, channel, start, end);
        if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_PROVIDER && !addedEvents)
            AddEvents(1, events, channel, start, end);
    }

    if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_XMLTV ||
        m_guidePreference == SC::Settings::GUIDE_PREFERENCE_XMLTV_ONLY) {
        addedEvents = AddEvents(1, events, channel, start, end);
        if (m_guidePreference == SC::Settings::GUIDE_PREFERENCE_PREFER_XMLTV && !addedEvents)
            AddEvents(0, events, channel, start, end);
    }

    return events;
}

void GuideManager::Clear() {
    m_epgData.clear();
    m_xmltv->Clear();
}
