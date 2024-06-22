/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "InstanceSettings.h"

using namespace Stalker;

InstanceSettings::InstanceSettings(kodi::addon::IAddonInstance& instance)
  : m_instance(instance)
{
  ReadSettings();
}

void InstanceSettings::ReadSettings()
{
  if (!m_instance.CheckInstanceSettingInt("connection_timeout", connectionTimeout))
    connectionTimeout = SC_SETTINGS_DEFAULT_CONNECTION_TIMEOUT;
  if (!m_instance.CheckInstanceSettingString("mac", mac))
    mac = SC_SETTINGS_DEFAULT_MAC;
  if (!m_instance.CheckInstanceSettingString("server", server))
    server = SC_SETTINGS_DEFAULT_MAC;
  if (!m_instance.CheckInstanceSettingString("time_zone", timeZone))
    timeZone = SC_SETTINGS_DEFAULT_TIME_ZONE;
  if (!m_instance.CheckInstanceSettingFloat("epg_timeshift", epgTimeshiftHours))
    epgTimeshiftHours = SC_SETTINGS_DEFAULT_EPG_TIMESHIFT;

  if (!m_instance.CheckInstanceSettingString("login", login))
    login = SC_SETTINGS_DEFAULT_LOGIN;
  if (!m_instance.CheckInstanceSettingString("password", password))
    password = SC_SETTINGS_DEFAULT_PASSWORD;

  if (!m_instance.CheckInstanceSettingEnum<InstanceSettings::GuidePreference>("guide_preference", guidePreference))
    guidePreference = GuidePreference::GUIDE_PREFERENCE_PREFER_PROVIDER;
  if (!m_instance.CheckInstanceSettingBoolean("guide_cache", guideCache))
    guideCache = SC_SETTINGS_DEFAULT_GUIDE_CACHE;
  if (!m_instance.CheckInstanceSettingInt("guide_cache_hours", guideCacheHours))
    guideCacheHours = SC_SETTINGS_DEFAULT_GUIDE_CACHE_HOURS;

  if (!m_instance.CheckInstanceSettingEnum<HTTPSocket::Scope>("xmltv_scope", xmltvScope))
    xmltvScope = HTTPSocket::Scope::SCOPE_REMOTE;
  if (xmltvScope == HTTPSocket::Scope::SCOPE_REMOTE)
    m_instance.CheckInstanceSettingString("xmltv_url", xmltvPath);
  else
    m_instance.CheckInstanceSettingString("xmltv_path", xmltvPath);

  if (!m_instance.CheckInstanceSettingString("token", token))
    token = SC_SETTINGS_DEFAULT_TOKEN;
  if (!m_instance.CheckInstanceSettingString("serial_number", serialNumber))
    serialNumber = SC_SETTINGS_DEFAULT_SERIAL_NUMBER;
  if (!m_instance.CheckInstanceSettingString("device_id", deviceId))
    deviceId = SC_SETTINGS_DEFAULT_DEVICE_ID;
  if (!m_instance.CheckInstanceSettingString("device_id2", deviceId2))
    deviceId2 = SC_SETTINGS_DEFAULT_DEVICE_ID2;
  if (!m_instance.CheckInstanceSettingString("signature", signature))
    signature = SC_SETTINGS_DEFAULT_SIGNATURE;

  kodi::Log(ADDON_LOG_DEBUG, "connection_timeout=%d", connectionTimeout);

  kodi::Log(ADDON_LOG_DEBUG, "mac=%s", mac.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "server=%s", server.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "timeZone=%s", timeZone.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "epgTimeshift=%f", epgTimeshiftHours);
  kodi::Log(ADDON_LOG_DEBUG, "login=%s", login.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "password=%s", password.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "guidePreference=%d", guidePreference);
  kodi::Log(ADDON_LOG_DEBUG, "guideCache=%d", guideCache);
  kodi::Log(ADDON_LOG_DEBUG, "guideCacheHours=%d", guideCacheHours);
  kodi::Log(ADDON_LOG_DEBUG, "xmltvScope=%d", xmltvScope);
  kodi::Log(ADDON_LOG_DEBUG, "xmltvPath=%s", xmltvPath.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "token=%s", token.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "serialNumber=%s", serialNumber.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "deviceId=%s", deviceId.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "deviceId2=%s", deviceId2.c_str());
  kodi::Log(ADDON_LOG_DEBUG, "signature=%s", signature.c_str());
}

ADDON_STATUS InstanceSettings::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  return ADDON_STATUS_OK;
}
