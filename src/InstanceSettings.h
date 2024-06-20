/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "HTTPSocket.h"

#include <kodi/AddonBase.h>

#include <string>

#define SC_SETTINGS_DEFAULT_ACTIVE_PORTAL 0
#define SC_SETTINGS_DEFAULT_MAC "00:1A:79:00:00:00"
#define SC_SETTINGS_DEFAULT_SERVER "127.0.0.1"
#define SC_SETTINGS_DEFAULT_TIME_ZONE "Europe/Kiev"
#define SC_SETTINGS_DEFAULT_EPG_TIMESHIFT 0.0f
#define SC_SETTINGS_DEFAULT_LOGIN ""
#define SC_SETTINGS_DEFAULT_PASSWORD ""
#define SC_SETTINGS_DEFAULT_CONNECTION_TIMEOUT 5 // 5 seconds
#define SC_SETTINGS_DEFAULT_GUIDE_PREFERENCE 0 // prefer provider
#define SC_SETTINGS_DEFAULT_GUIDE_CACHE 1 // true
#define SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS 24
#define SC_SETTINGS_DEFAULT_XMLTV_SCOPE 0 // remote url
#define SC_SETTINGS_DEFAULT_XMLTV_URL ""
#define SC_SETTINGS_DEFAULT_XMLTV_PATH ""
#define SC_SETTINGS_DEFAULT_TOKEN ""
#define SC_SETTINGS_DEFAULT_SERIAL_NUMBER ""
#define SC_SETTINGS_DEFAULT_DEVICE_ID ""
#define SC_SETTINGS_DEFAULT_DEVICE_ID2 ""
#define SC_SETTINGS_DEFAULT_SIGNATURE ""

namespace SC
{
class InstanceSettings
{
public:
  typedef enum
  {
    GUIDE_PREFERENCE_PREFER_PROVIDER,
    GUIDE_PREFERENCE_PREFER_XMLTV,
    GUIDE_PREFERENCE_PROVIDER_ONLY,
    GUIDE_PREFERENCE_XMLTV_ONLY
  } GuidePreference;

  explicit InstanceSettings(kodi::addon::IAddonInstance& instance);

  void ReadSettings();
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

  int activePortal;
  std::string mac;
  std::string server;
  std::string timeZone;
  float epgTimeshiftHours = 0.0f;
  std::string login;
  std::string password;
  int connectionTimeout;
  GuidePreference guidePreference;
  bool guideCache;
  int guideCacheHours;
  HTTPSocket::Scope xmltvScope;
  std::string xmltvPath;
  std::string token;
  std::string serialNumber;
  std::string deviceId;
  std::string deviceId2;
  std::string signature;

  kodi::addon::IAddonInstance& m_instance;
};
} // namespace SC
