#pragma once

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

#include "base/ChannelManager.h"
#include "Error.h"
#include "SAPI.h"

namespace SC {
    struct Channel : Base::Channel {
        std::string streamUrl;
        std::string iconPath;
        int channelId;
        std::string cmd;
        std::string tvGenreId;
        bool useHttpTmpLink;
        bool useLoadBalancing;
    };

    struct ChannelGroup {
        std::string id;
        std::string name;
        std::string alias;
    };

    class ChannelManager : public Base::ChannelManager<Channel> {
    public:
        ChannelManager();

        virtual ~ChannelManager();

        virtual void SetAPI(SAPI *api) {
            m_api = api;
        }

        virtual SError LoadChannels();

        virtual SError LoadChannelGroups();

        virtual ChannelGroup *GetChannelGroup(const std::string &name) {
            std::vector<ChannelGroup>::iterator cgIt;
            cgIt = std::find_if(m_channelGroups.begin(), m_channelGroups.end(), [&name](const ChannelGroup &cg) {
                return !cg.name.compare(name);
            });
            return cgIt != m_channelGroups.end() ? &(*cgIt) : nullptr;
        }

        virtual std::vector<ChannelGroup> GetChannelGroups() {
            return m_channelGroups;
        }

        virtual std::string GetStreamURL(Channel &channel);

    private:
        bool ParseChannels(Json::Value &parsed);

        //https://github.com/kodi-pvr/pvr.iptvsimple/blob/master/src/PVRIptvData.cpp#L1085
        int GetChannelId(const char *strChannelName, const char *strStreamUrl);

        bool ParseChannelGroups(Json::Value &parsed);

        std::string ParseStreamCmd(Json::Value &parsed);

        SAPI *m_api;
        std::vector<ChannelGroup> m_channelGroups;
    };
}
