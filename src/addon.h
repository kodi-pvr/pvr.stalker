/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Sam Stenvall
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

#include <memory>

#include "AddonSettings.h"

namespace SC
{
class StalkerInstance;
}

class StalkerInstance;

class ATTR_DLL_LOCAL CStalkerAddon : public kodi::addon::CAddonBase
{
public:
  CStalkerAddon() = default;

  ADDON_STATUS Create() override;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl) override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue) override;
  void DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl) override;

private:

  SC::StalkerInstance* m_stalker = nullptr;
  std::shared_ptr<SC::AddonSettings> m_settings;
};
