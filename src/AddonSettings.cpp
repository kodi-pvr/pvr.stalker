/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "AddonSettings.h"

#include "SettingsMigration.h"

#include "kodi/General.h"

using namespace Stalker;

AddonSettings::AddonSettings()
{
  ReadSettings();
}

void AddonSettings::ReadSettings()
{
  // This add-on only has instance settings!
}

ADDON_STATUS AddonSettings::SetSetting(const std::string& settingName,
                                       const kodi::addon::CSettingValue& settingValue)
{
  if (SettingsMigration::IsMigrationSetting(settingName))
  {
    // ignore settings from pre-multi-instance setup
    return ADDON_STATUS_OK;
  }

  kodi::Log(ADDON_LOG_ERROR, "AddonSettings::SetSetting - unknown setting '%s'",
              settingName.c_str());
  return ADDON_STATUS_UNKNOWN;
}