#pragma once

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

#include <thread>
#include <vector>

#include <json/json.h>
#include <p8-platform/threads/mutex.h>

#include "libstalkerclient/identity.h"
#include "libstalkerclient/stb.h"
#include "base/Cache.h"
#include "ChannelManager.h"
#include "client.h"
#include "CWatchdog.h"
#include "Error.h"
#include "GuideManager.h"
#include "SAPI.h"
#include "SessionManager.h"
#include "Settings.h"
#include "XMLTV.h"

class SData : Base::Cache {
public:
    SData();

    ~SData();

    bool ReloadSettings();

    PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t anEnd);

    int GetChannelGroupsAmount();

    PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool radio);

    PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

    int GetChannelsAmount();

    PVR_ERROR GetChannels(ADDON_HANDLE handle, bool radio);

    PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

    SC::Settings settings;
protected:
    bool LoadCache();

    bool SaveCache();

    bool IsAuthenticated() const;

    SError Authenticate();

    void QueueErrorNotification(SError error) const;

private:
    std::string GetChannelStreamURL(const PVR_CHANNEL &channel) const;

    bool m_tokenManuallySet;
    time_t m_lastEpgAccessTime;
    time_t m_nextEpgLoadTime;
    sc_identity_t m_identity;
    sc_stb_profile_t m_profile;
    bool m_epgThreadActive;
    std::thread m_epgThread;
    P8PLATFORM::CMutex m_epgMutex;
    SC::SAPI *m_api;
    SC::SessionManager *m_sessionManager;
    SC::ChannelManager *m_channelManager;
    SC::GuideManager *m_guideManager;
};
