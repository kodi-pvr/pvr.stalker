/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SettingsMigration.h"

#include "kodi/General.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace SC;

namespace
{
// <setting name, default value> maps
const std::vector<std::pair<const char*, const char*>> stringMap = {{"mac", "0:1A:79:00:00:00"},
                                                                    {"server", "127.0.0.1"},
                                                                    {"time_zone", "Europe/Kiev"},
                                                                    {"login", ""},
                                                                    {"password", ""},
                                                                    {"xmltv_url", ""},
                                                                    {"xmltv_path", ""},
                                                                    {"token", ""},
                                                                    {"serial_number", ""},
                                                                    {"device_id", ""},
                                                                    {"device_id2", ""},
                                                                    {"signature", ""}};

const std::vector<std::pair<const char*, int>> intMap = {{"connection_timeout", 5},
                                                         {"guide_preference", 1},
                                                         {"guide_cache_hours", 24},
                                                         {"xmltv_scope", 0}};

const std::vector<std::pair<const char*, float>> floatMap = {{"epg_timeshift", 0.0f}};

const std::vector<std::pair<const char*, bool>> boolMap = {{"guide_cache", true}};

} // unnamed namespace

bool SettingsMigration::MigrateSettings(kodi::addon::IAddonInstance& target)
{
  std::string stringValue;
  bool boolValue{false};
  int intValue{0};

  if (target.CheckInstanceSettingString("kodi_addon_instance_name", stringValue) &&
      !stringValue.empty())
  {
    // Instance already has valid instance settings
    return false;
  }

  // Read pre-multi-instance settings from settings.xml, transfer to instance settings
  SettingsMigration mig(target);

  for (const auto& setting : stringMap)
    mig.MigrateStringSetting(setting.first, setting.second);

  for (const auto& setting : intMap)
    mig.MigrateIntSetting(setting.first, setting.second);

  for (const auto& setting : floatMap)
    mig.MigrateFloatSetting(setting.first, setting.second);

  for (const auto& setting : boolMap)
    mig.MigrateBoolSetting(setting.first, setting.second);

  if (mig.Changed())
  {
    // Set a title for the new instance settings
    std::string title = "Migrated Add-on Config";
    target.SetInstanceSettingString("kodi_addon_instance_name", title);

    return true;
  }
  return false;
}

bool SettingsMigration::IsMigrationSetting(const std::string& key)
{
  std::string oldSettingsKey{key};
  oldSettingsKey += "_0";

  return std::any_of(stringMap.cbegin(), stringMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(intMap.cbegin(), intMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(floatMap.cbegin(), floatMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(boolMap.cbegin(), boolMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(stringMap.cbegin(), stringMap.cend(),
                     [&oldSettingsKey](const auto& entry) { return entry.first == oldSettingsKey; }) ||
         std::any_of(intMap.cbegin(), intMap.cend(),
                     [&oldSettingsKey](const auto& entry) { return entry.first == oldSettingsKey; }) ||
         std::any_of(floatMap.cbegin(), floatMap.cend(),
                     [&oldSettingsKey](const auto& entry) { return entry.first == oldSettingsKey; }) ||
         std::any_of(boolMap.cbegin(), boolMap.cend(),
                     [&oldSettingsKey](const auto& entry) { return entry.first == oldSettingsKey; });
}

void SettingsMigration::MigrateStringSetting(const char* key, const std::string& defaultValue)
{
  std::string oldSettingsKey{key};
  oldSettingsKey += "_0";

  std::string value;
  if (kodi::addon::CheckSettingString(oldSettingsKey, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingString(key, value);
    m_changed = true;
  }
  else if (kodi::addon::CheckSettingString(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingString(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateIntSetting(const char* key, int defaultValue)
{
  std::string oldSettingsKey{key};
  oldSettingsKey += "_0";

  // Stalker uses the old settings format prior to migration and reading int and boolean reliably requires reading into
  // a string and then into a int. Luckily after this messy migration, the settings will be in the new format going forward.
  int value;
  std::string stringValue;
  if (kodi::addon::CheckSettingString(oldSettingsKey, stringValue) && stringValue != std::to_string(defaultValue))
  {
    value = std::atoi(stringValue.c_str());
    m_target.SetInstanceSettingInt(key, value);
    m_changed = true;
  }
  else if (kodi::addon::CheckSettingString(key, stringValue) && stringValue != std::to_string(defaultValue))
  {
    value = std::atoi(stringValue.c_str());
    if (oldSettingsKey == "connection_timeout_0")
      value *= 5;

    m_target.SetInstanceSettingInt(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateFloatSetting(const char* key, float defaultValue)
{
  std::string oldSettingsKey{key};
  oldSettingsKey += "_0";

  float value;
  if (kodi::addon::CheckSettingFloat(oldSettingsKey, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingFloat(key, value);
    m_changed = true;
  }
  else if (kodi::addon::CheckSettingFloat(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingFloat(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateBoolSetting(const char* key, bool defaultValue)
{
  std::string oldSettingsKey{key};
  oldSettingsKey += "_0";

  // Stalker uses the old settings format prior to migration and reading int and boolean reliably requires reading into
  // a string and then into a int. Luckily after this messy migration, the settings will be in the new format going forward.
  bool value;
  std::string stringValue;
  if (kodi::addon::CheckSettingString(oldSettingsKey, stringValue) && stringValue != (defaultValue ? "true" : "false"))
  {
    value = stringValue == "true";
    m_target.SetInstanceSettingBoolean(key, value);
    m_changed = true;
  }
}
