/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Sam Stenvall
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "addon.h"
#include "StalkerInstance.h"
#include "SettingsMigration.h"

using namespace SC;

ADDON_STATUS CStalkerAddon::Create()
{
  /* Init settings */
  m_settings.reset(new AddonSettings());

  kodi::Log(ADDON_LOG_DEBUG,  "%s starting PVR client...", __func__);

  return ADDON_STATUS_OK;
}

ADDON_STATUS CStalkerAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    kodi::Log(ADDON_LOG_DEBUG, "creating Stalker Portal PVR addon");

    m_stalker = new SC::StalkerInstance(instance);
    ADDON_STATUS status = m_stalker->Initialize();

    // Try to migrate settings from a pre-multi-instance setup
    if (SettingsMigration::MigrateSettings(*m_stalker))
    {
      // Initial client operated on old/incomplete settings
      delete m_stalker;
      m_stalker = new SC::StalkerInstance(instance);
    }

    hdl = m_stalker;

    return status;
  }

  return ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS CStalkerAddon::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  return m_settings->SetSetting(settingName, settingValue);
}

void CStalkerAddon::DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    m_stalker = nullptr;
  }
}

ADDONCREATOR(CStalkerAddon)
