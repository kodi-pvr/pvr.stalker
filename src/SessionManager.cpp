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

using namespace SC;

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
  bool wasAuthenticated(m_authenticated);
  int maxRetires(5);
  int numRetries(0);

  if (m_isAuthenticating)
    return SERROR_OK;

  StopWatchdog();

  m_authMutex.lock();
  m_isAuthenticating = true;
  m_authenticated = false;
  m_lastUnknownError.clear();
  m_authMutex.unlock();

  if (wasAuthenticated && m_statusCallback != nullptr)
    m_statusCallback(SERROR_AUTHORIZATION);

  while (!m_authenticated && ++numRetries <= maxRetires)
  {
    // notify once after the first try failed
    if (numRetries == 2)
    {
      if (m_statusCallback != nullptr)
        m_statusCallback(SERROR_AUTHENTICATION);
    }

    // don't sleep on first try
    if (numRetries > 1)
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    if (!m_hasUserDefinedToken && SERROR_OK != DoHandshake())
      continue;

    if (SERROR_OK != GetProfile())
      continue;

    m_authMutex.lock();
    m_authenticated = true;
    m_isAuthenticating = false;
    m_authMutex.unlock();

    if (wasAuthenticated && m_statusCallback != nullptr)
      m_statusCallback(SERROR_OK);
  }

  if (m_authenticated)
  {
    StartAuthInvoker();
    StartWatchdog();
  }

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
  if (!m_watchdog)
  {
    m_watchdog = new CWatchdog((unsigned int)m_profile->timeslot, m_api, [this](SError err) {
      if (err == SERROR_AUTHORIZATION)
      {
        m_authMutex.lock();
        m_authenticated = false;
        m_authMutex.unlock();
      }
    });
  }

  if (m_watchdog)
    m_watchdog->Start();
}

void SessionManager::StopWatchdog()
{
  if (m_watchdog)
    m_watchdog->Stop();
}
