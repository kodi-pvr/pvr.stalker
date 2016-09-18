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

#include "ChannelManager.h"

#include <cmath>

#include "client.h"
#include "Utils.h"

using namespace ADDON;
using namespace SC;

ChannelManager::ChannelManager() : Base::ChannelManager<Channel>() {
    m_api = nullptr;
}

ChannelManager::~ChannelManager() {
    m_api = nullptr;
    m_channelGroups.clear();
}

SError ChannelManager::LoadChannels() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    Json::Value parsed;
    int genre = 10;
    unsigned int currentPage = 1;
    unsigned int maxPages = 1;

    if (!m_api->ITVGetAllChannels(parsed) || !ParseChannels(parsed)) {
        XBMC->Log(LOG_ERROR, "%s: ITVGetAllChannels failed", __FUNCTION__);
        return SERROR_LOAD_CHANNELS;
    }

    while (currentPage <= maxPages) {
        XBMC->Log(LOG_DEBUG, "%s: currentPage: %d", __FUNCTION__, currentPage);

        if (!m_api->ITVGetOrderedList(genre, currentPage, parsed) || !ParseChannels(parsed)) {
            XBMC->Log(LOG_ERROR, "%s: ITVGetOrderedList failed", __FUNCTION__);
            return SERROR_LOAD_CHANNELS;
        }

        if (currentPage == 1) {
            int totalItems = Utils::GetIntFromJsonValue(parsed["js"]["total_items"]);
            int maxPageItems = Utils::GetIntFromJsonValue(parsed["js"]["max_page_items"]);

            if (totalItems > 0 && maxPageItems > 0)
                maxPages = static_cast<unsigned int>(ceil((double) totalItems / maxPageItems));

            XBMC->Log(LOG_DEBUG, "%s: totalItems: %d | maxPageItems: %d | maxPages: %d", __FUNCTION__, totalItems,
                      maxPageItems, maxPages);
        }

        currentPage++;
    }

    return SERROR_OK;
}

bool ChannelManager::ParseChannels(Json::Value &parsed) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    if (!parsed.isMember("js") || !parsed["js"].isMember("data"))
        return false;

    Json::Value value;

    try {
        value = parsed["js"]["data"];
        if (!value.isObject() && !value.isArray())
            return false;

        for (Json::Value::iterator it = value.begin(); it != value.end(); ++it) {
            Channel channel;
            channel.uniqueId = GetChannelId((*it)["name"].asCString(), (*it)["number"].asCString());
            channel.number = Utils::StringToInt((*it)["number"].asString());
            channel.name = (*it)["name"].asString();

            // "pvr://stream/" causes GetLiveStreamURL to be called
            channel.streamUrl = "pvr://stream/" + Utils::ToString(channel.uniqueId);

            std::string strLogo = (*it)["logo"].asString();
            channel.iconPath = Utils::DetermineLogoURI(m_api->GetBasePath(), strLogo);

            channel.channelId = Utils::GetIntFromJsonValue((*it)["id"]);
            channel.cmd = (*it)["cmd"].asString();
            channel.tvGenreId = (*it)["tv_genre_id"].asString();
            channel.useHttpTmpLink = !!Utils::GetIntFromJsonValue((*it)["use_http_tmp_link"]);
            channel.useLoadBalancing = !!Utils::GetIntFromJsonValue((*it)["use_load_balancing"]);

            m_channels.push_back(channel);

            XBMC->Log(LOG_DEBUG, "%s: %d - %s", __FUNCTION__, channel.number, channel.name.c_str());
        }
    } catch (const std::exception &ex) {
        return false;
    }

    return true;
}

//https://github.com/kodi-pvr/pvr.iptvsimple/blob/master/src/PVRIptvData.cpp#L1085
int ChannelManager::GetChannelId(const char *strChannelName, const char *strStreamUrl) {
    std::string concat(strChannelName);
    concat.append(strStreamUrl);

    const char *strString = concat.c_str();
    int iId = 0;
    int c;
    while ((c = *strString++))
        iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

    return abs(iId);
}

SError ChannelManager::LoadChannelGroups() {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    Json::Value parsed;

    // genres are channel groups
    if (!m_api->ITVGetGenres(parsed) || !ParseChannelGroups(parsed)) {
        XBMC->Log(LOG_ERROR, "%s: ITVGetGenres|ParseChannelGroups failed", __FUNCTION__);
        return SERROR_LOAD_CHANNEL_GROUPS;
    }

    return SERROR_OK;
}

bool ChannelManager::ParseChannelGroups(Json::Value &parsed) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    if (!parsed.isMember("js"))
        return false;

    Json::Value value;

    try {
        value = parsed["js"];
        if (!value.isObject() && !value.isArray())
            return false;

        for (Json::Value::iterator it = value.begin(); it != value.end(); ++it) {
            ChannelGroup channelGroup;
            channelGroup.name = (*it)["title"].asString();
            if (!channelGroup.name.empty())
                channelGroup.name[0] = (char) toupper(channelGroup.name[0]);
            channelGroup.id = (*it)["id"].asString();
            channelGroup.alias = (*it)["alias"].asString();

            m_channelGroups.push_back(channelGroup);

            XBMC->Log(LOG_DEBUG, "%s: %s - %s", __FUNCTION__, channelGroup.id.c_str(), channelGroup.name.c_str());
        }
    } catch (const std::exception &ex) {
        return false;
    }

    return true;
}

std::string ChannelManager::GetStreamURL(Channel &channel) {
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

    std::string cmd;
    Json::Value parsed;
    size_t pos;

    // /c/player.js#L2198
    if (channel.useHttpTmpLink || channel.useLoadBalancing) {
        XBMC->Log(LOG_DEBUG, "%s: getting temp stream url", __FUNCTION__);

        if (!m_api->ITVCreateLink(channel.cmd, parsed)) {
            XBMC->Log(LOG_ERROR, "%s: ITVCreateLink failed", __FUNCTION__);
            return cmd;
        }

        cmd = ParseStreamCmd(parsed);
    } else {
        cmd = channel.cmd;
    }

    // cmd format
    // (?:ffrt\d*\s|)(.*)
    if ((pos = cmd.find(" ")) != std::string::npos)
        cmd = cmd.substr(pos + 1);

    return cmd;
}

std::string ChannelManager::ParseStreamCmd(Json::Value &parsed) {
    std::string cmd;

    if (parsed.isMember("js") && parsed["js"].isMember("cmd"))
        cmd = parsed["js"]["cmd"].asString();

    return cmd;
}
