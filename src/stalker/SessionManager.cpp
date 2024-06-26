/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SessionManager.h"

#include "Utils.h"

#include <chrono>
#include <kodi/General.h>
#include <thread>

using namespace Stalker;

SessionManager::~SessionManager()
{
  if (m_watchdog)
  {
    StopWatchdog();
    delete m_watchdog;
  }

  StopAuthInvoker();
}

SError SessionManager::DoHandshake()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  Json::Value parsed;

  if (!m_api->STBHandshake(parsed))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: STBHandshake failed", __func__);
    return SERROR_AUTHENTICATION;
  }

  if (parsed["js"].isMember("token"))
    SC_STR_SET(m_identity->token, parsed["js"]["token"].asCString());

  kodi::Log(ADDON_LOG_DEBUG, "%s: token=%s", __func__, m_identity->token);

  if (parsed["js"].isMember("not_valid"))
    m_identity->valid_token = !Utils::GetIntFromJsonValue(parsed["js"]["not_valid"]);

  return SERROR_OK;
}

SError SessionManager::DoAuth()
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  Json::Value parsed;
  SError ret(SERROR_OK);

  if (!m_api->STBDoAuth(parsed))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: STBDoAuth failed", __func__);
    return SERROR_AUTHENTICATION;
  }

  if (parsed.isMember("js") && !parsed["js"].asBool())
    ret = SERROR_AUTHENTICATION;

  return ret;
}

SError SessionManager::GetProfile(bool authSecondStep)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  Json::Value parsed;
  SError ret(SERROR_OK);

  if (!m_api->STBGetProfile(authSecondStep, parsed))
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: STBGetProfile failed", __func__);
    return SERROR_AUTHENTICATION;
  }

  sc_stb_profile_defaults(m_profile);

  if (parsed["js"].isMember("store_auth_data_on_stb"))
    m_profile->store_auth_data_on_stb =
        Utils::GetBoolFromJsonValue(parsed["js"]["store_auth_data_on_stb"]);

  if (parsed["js"].isMember("status"))
    m_profile->status = Utils::GetIntFromJsonValue(parsed["js"]["status"]);

  SC_STR_SET(m_profile->msg, !parsed["js"].isMember("msg") ? "" : parsed["js"]["msg"].asCString());

  SC_STR_SET(m_profile->block_msg,
             !parsed["js"].isMember("block_msg") ? "" : parsed["js"]["block_msg"].asCString());

  if (parsed["js"].isMember("watchdog_timeout"))
    m_profile->watchdog_timeout = Utils::GetIntFromJsonValue(parsed["js"]["watchdog_timeout"]);

  if (parsed["js"].isMember("timeslot"))
    m_profile->timeslot = Utils::GetDoubleFromJsonValue(parsed["js"]["timeslot"]);

  kodi::Log(ADDON_LOG_DEBUG, "%s: timeslot=%f", __func__, m_profile->timeslot);

  switch (m_profile->status)
  {
    case 0:
      break;
    case 2:
      ret = DoAuth();
      if (ret != SERROR_OK)
        return ret;

      return GetProfile(true);
    case 1:
    default:
      m_lastUnknownError = m_profile->msg;
      kodi::Log(ADDON_LOG_ERROR, "%s: status=%i | msg=%s | block_msg=%s", __func__,
                m_profile->status, m_profile->msg, m_profile->block_msg);
      return SERROR_UNKNOWN;
  }

  return ret;
}

SError SessionManager::Authenticate()
{
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 1");
  bool wasAuthenticated(m_authenticated);
  int maxRetires(5);
  int numRetries(0);

  if (m_isAuthenticating)
    return SERROR_OK;

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 2");

  StopWatchdog();

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3");

  m_authMutex.lock();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3a");
  m_isAuthenticating = true;
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3b");
  m_authenticated = false;
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3c");
  m_lastUnknownError.clear();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3d");
  m_authMutex.unlock();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 3e");

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 4");

  if (wasAuthenticated && m_statusCallback != nullptr)
    m_statusCallback(SERROR_AUTHORIZATION);

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 5");

  while (!m_authenticated && ++numRetries <= maxRetires)
  {
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 6");

    // notify once after the first try failed
    if (numRetries == 2)
    {
      if (m_statusCallback != nullptr)
        m_statusCallback(SERROR_AUTHENTICATION);
    }

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 7");

    // don't sleep on first try
    if (numRetries > 1)
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 8");

    if (!m_hasUserDefinedToken && SERROR_OK != DoHandshake())
      continue;

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 9");

    if (SERROR_OK != GetProfile())
      continue;

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 10");

    m_authMutex.lock();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 10a");
    m_authenticated = true;
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 10b");
    m_isAuthenticating = false;
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 10c");
    m_authMutex.unlock();

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 11");


    if (wasAuthenticated && m_statusCallback != nullptr)
      m_statusCallback(SERROR_OK);

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 12");

  }

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 13");

  if (m_authenticated)
  {
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 14");
    StartAuthInvoker();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 15");
    StartWatchdog();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 16");
  }

  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::Authenticate 17");

  return SERROR_OK;
}

void SessionManager::StartAuthInvoker()
{
  m_threadActive = true;
  if (m_thread.joinable())
    return;

  m_thread = std::thread([this] {
    unsigned int target(30000);
    unsigned int count;

    while (m_threadActive)
    {
      if (!m_authenticated)
        Authenticate();

      count = 0;
      while (count < target)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!m_threadActive)
          break;
        count += 100;
      }
    }
  });
}

void SessionManager::StopAuthInvoker()
{
  m_threadActive = false;
  if (m_thread.joinable())
    m_thread.join();
}

void SessionManager::StartWatchdog()
{
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 1");
  if (!m_watchdog)
  {
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 2");
    m_watchdog = new CWatchdog((unsigned int)m_profile->timeslot, m_api, [this](SError err) {
      if (err == SERROR_AUTHORIZATION)
      {
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 3");
        m_authMutex.lock();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 4");
        m_authenticated = false;
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 5");
        m_authMutex.unlock();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 6");
      }
    });
  }
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 7");

  if (m_watchdog)
    m_watchdog->Start();
  kodi::Log(ADDON_LOG_DEBUG, "ZZ SessionManager::StartWatchdog 8");
}

void SessionManager::StopWatchdog()
{
  if (m_watchdog)
    m_watchdog->Stop();
}
