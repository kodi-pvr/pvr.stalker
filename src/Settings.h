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

#include <string>

#include "HTTPSocket.h"

#define SC_SETTINGS_DEFAULT_ACTIVE_PORTAL       0
#define SC_SETTINGS_DEFAULT_MAC                 "00:1A:79:00:00:00"
#define SC_SETTINGS_DEFAULT_SERVER              "127.0.0.1"
#define SC_SETTINGS_DEFAULT_TIME_ZONE           "Europe/Kiev"
#define SC_SETTINGS_DEFAULT_LOGIN               ""
#define SC_SETTINGS_DEFAULT_PASSWORD            ""
#define SC_SETTINGS_DEFAULT_CONNECTION_TIMEOUT  1 // 5 seconds
#define SC_SETTINGS_DEFAULT_GUIDE_PREFERENCE    0 // prefer provider
#define SC_SETTINGS_DEFAULT_GUIDE_CACHE         1 // true
#define SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS   24
#define SC_SETTINGS_DEFAULT_XMLTV_SCOPE         0 // remote url
#define SC_SETTINGS_DEFAULT_XMLTV_URL           ""
#define SC_SETTINGS_DEFAULT_XMLTV_PATH          ""
#define SC_SETTINGS_DEFAULT_TOKEN               ""
#define SC_SETTINGS_DEFAULT_SERIAL_NUMBER       ""
#define SC_SETTINGS_DEFAULT_DEVICE_ID           ""
#define SC_SETTINGS_DEFAULT_DEVICE_ID2          ""
#define SC_SETTINGS_DEFAULT_SIGNATURE           ""

namespace SC {
    class Settings {
    public:
        typedef enum {
            GUIDE_PREFERENCE_PREFER_PROVIDER,
            GUIDE_PREFERENCE_PREFER_XMLTV,
            GUIDE_PREFERENCE_PROVIDER_ONLY,
            GUIDE_PREFERENCE_XMLTV_ONLY
        } GuidePreference;

        int activePortal;
        std::string mac;
        std::string server;
        std::string timeZone;
        std::string login;
        std::string password;
        unsigned int connectionTimeout;
        GuidePreference guidePreference;
        bool guideCache;
        unsigned int guideCacheHours;
        HTTPSocket::Scope xmltvScope;
        std::string xmltvPath;
        std::string token;
        std::string serialNumber;
        std::string deviceId;
        std::string deviceId2;
        std::string signature;
    };
}
