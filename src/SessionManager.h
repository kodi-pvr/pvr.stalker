/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <mutex>

#include "libstalkerclient/stb.h"
#include "CWatchdog.h"
#include "Error.h"
#include "SAPI.h"

namespace SC {
    class SessionManager {
    public:
        SessionManager();

        virtual ~SessionManager();

        virtual void SetIdentity(sc_identity_t *identity, bool hasUserDefinedToken = false) {
            m_identity = identity;
            m_hasUserDefinedToken = hasUserDefinedToken;
        }

        virtual void SetProfile(sc_stb_profile_t *profile) {
            m_profile = profile;
        }

        virtual void SetAPI(SAPI *api) {
            m_api = api;
        }

        virtual void SetStatusCallback(std::function<void(SError)> statusCallback) {
            m_statusCallback = statusCallback;
        }

        virtual std::string GetLastUnknownError() {
            std::string tmp = m_lastUnknownError;
            m_lastUnknownError.clear();
            return tmp;
        }

        virtual bool IsAuthenticated() {
            return m_authenticated && !m_isAuthenticating;
        }

        virtual SError Authenticate();

    private:
        SError DoHandshake();

        SError DoAuth();

        SError GetProfile(bool authSecondStep = false);

        void StartAuthInvoker();

        void StopAuthInvoker();

        void StartWatchdog();

        void StopWatchdog();

        sc_identity_t *m_identity;
        bool m_hasUserDefinedToken;
        sc_stb_profile_t *m_profile;
        SAPI *m_api;
        std::function<void(SError)> m_statusCallback;
        std::string m_lastUnknownError;
        bool m_authenticated;
        bool m_isAuthenticating;
        std::mutex m_authMutex;
        CWatchdog *m_watchdog;
        bool m_threadActive;
        std::thread m_thread;
    };
}
