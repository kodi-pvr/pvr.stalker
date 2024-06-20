/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include "kodi/AddonBase.h"

namespace SC
{
/**
   * Represents the current addon settings
   */
class AddonSettings
{
  public:
    AddonSettings();

    /**
     * Set a value according to key definition in settings.xml
     */
    ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

  private:
    AddonSettings(const AddonSettings&) = delete;
    void operator=(const AddonSettings&) = delete;

    /**
     * Read all settings defined in settings.xml
     */
    void ReadSettings();
};

} // namespace SC