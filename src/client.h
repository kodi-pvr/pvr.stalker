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
#include "libXBMC_gui.h"

#define DEFAULT_MAC       "00:1A:79:00:00:00"
#define DEFAULT_SERVER    "127.0.0.1"
#define DEFAULT_TIME_ZONE "Europe/Kiev"

extern std::string  g_strUserPath;
extern std::string  g_strClientPath;

extern std::string  g_strMac;
extern std::string  g_strServer;
extern std::string  g_strTimeZone;
extern std::string  g_strApiBasePath;
extern std::string  g_api_endpoint;
extern std::string  g_referer;
extern std::string  g_token;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;
