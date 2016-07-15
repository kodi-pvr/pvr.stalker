#pragma once

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

#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

#define PORTAL_SUFFIX_FORMAT        "%s_%d"

#define DEFAULT_ACTIVE_PORTAL       0
#define DEFAULT_MAC                 "00:1A:79:00:00:00"
#define DEFAULT_SERVER              "127.0.0.1"
#define DEFAULT_TIME_ZONE           "Europe/Kiev"
#define DEFAULT_LOGIN               ""
#define DEFAULT_PASSWORD            ""
#define DEFAULT_CONNECTION_TIMEOUT  1 // 5 seconds
#define DEFAULT_GUIDE_PREFERENCE    0 // prefer provider
#define DEFAULT_GUIDE_CACHE         1 // true
#define DEFAULT_GUIDE_CACHE_HOURS   24
#define DEFAULT_XMLTV_SCOPE         0 // remote url
#define DEFAULT_XMLTV_URL           ""
#define DEFAULT_XMLTV_PATH          ""
#define DEFAULT_TOKEN               ""
#define DEFAULT_SERIAL_NUMBER       ""
#define DEFAULT_DEVICE_ID           ""
#define DEFAULT_DEVICE_ID2          ""
#define DEFAULT_SIGNATURE           ""


typedef enum {
  PREFER_PROVIDER,
  PREFER_XMLTV,
  PROVIDER_ONLY,
  XMLTV_ONLY
} GuidePreference;

typedef enum {
  REMOTE_URL,
  LOCAL_PATH
} XMLTVScope;

extern std::string  g_strUserPath;
extern std::string  g_strClientPath;

extern int          g_iActivePortal;
extern std::string  g_strMac;
extern std::string  g_strServer;
extern std::string  g_strTimeZone;
extern std::string  g_strLogin;
extern std::string  g_strPassword;
extern int          g_iConnectionTimeout;
extern int          g_iGuidePreference;
extern bool         g_bGuideCache;
extern int          g_iGuideCacheHours;
extern int          g_iXmltvScope;
extern std::string  g_strXmltvUrl;
extern std::string  g_strXmltvPath;
extern std::string  g_strToken;
extern std::string  g_strSerialNumber;
extern std::string  g_strDeviceId;
extern std::string  g_strDeviceId2;
extern std::string  g_strSignature;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;
