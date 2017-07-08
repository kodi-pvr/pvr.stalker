#pragma once

/*
 *      Copyright (C) 2015, 2016  Jamal Edey
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include <thread>
#include <functional>

#include "SAPI.h"

namespace SC {
    class CWatchdog {
    public:
        CWatchdog(unsigned int interval, SAPI *api, std::function<void(SError)> errorCallback);

        virtual ~CWatchdog();

        virtual void Start();

        virtual void Stop();

    private:
        void Process();

        unsigned int m_interval;
        SAPI *m_api;
        std::function<void(SError)> m_errorCallback;
        bool m_threadActive;
        std::thread m_thread;
    };
}
