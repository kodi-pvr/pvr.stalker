/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "CWatchdog.h"

#include "client.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define usleep(usec) Sleep((DWORD)(usec)/1000)
#else
#include <unistd.h>
#endif

using namespace ADDON;
using namespace SC;

CWatchdog::CWatchdog(uint32_t interval, SAPI *api, std::function<void(SError)> errorCallback)
        : m_interval(interval), m_api(api), m_errorCallback(errorCallback), m_threadActive(false) {
}

CWatchdog::~CWatchdog() {
    Stop();
}

void CWatchdog::Start() {
    m_threadActive = true;
    m_thread = std::thread([this] {
        Process();
    });
}

void CWatchdog::Stop() {
    m_threadActive = false;
    if (m_thread.joinable())
        m_thread.join();
}

void CWatchdog::Process() {
    XBMC->Log(LOG_DEBUG, "%s: start", __FUNCTION__);

    int curPlayType;
    int eventActiveId;
    Json::Value parsed;
    SError ret;
    unsigned int target(m_interval * 1000);
    unsigned int count;

    while (m_threadActive) {
        // hardcode values for now
        curPlayType = 1; // tv
        eventActiveId = 0;

        ret = m_api->WatchdogGetEvents(curPlayType, eventActiveId, parsed);
        if (ret != SERROR_OK) {
            XBMC->Log(LOG_ERROR, "%s: WatchdogGetEvents failed", __FUNCTION__);

            if (m_errorCallback != nullptr)
                m_errorCallback(ret);
        }

        // ignore the result. don't confirm events (yet)

        parsed.clear();

        count = 0;
        while (count < target) {
            usleep(100000);
            if (!m_threadActive)
                break;
            count += 100;
        }
    }

    XBMC->Log(LOG_DEBUG, "%s: stop", __FUNCTION__);
}
