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

#include <string>

#include <json/json.h>

#include "libstalkerclient/identity.h"
#include "libstalkerclient/request.h"
#include "Error.h"

namespace SC {
    class SAPI {
    public:
        SAPI();

        virtual ~SAPI();

        virtual void SetIdentity(sc_identity_t *identity) {
            m_identity = identity;
        }

        virtual void SetEndpoint(const std::string &endpoint);

        virtual std::string GetBasePath() {
            return m_basePath;
        }

        virtual void SetTimeout(unsigned int timeout) {
            m_timeout = timeout;
        }

        virtual bool STBHandshake(Json::Value &parsed);

        virtual bool STBGetProfile(bool authSecondStep, Json::Value &parsed);

        virtual bool STBDoAuth(Json::Value &parsed);

        virtual bool ITVGetAllChannels(Json::Value &parsed);

        virtual bool ITVGetOrderedList(int genre, int page, Json::Value &parsed);

        virtual bool ITVCreateLink(std::string &cmd, Json::Value &parsed);

        virtual bool ITVGetGenres(Json::Value &parsed);

        virtual bool ITVGetEPGInfo(int period, Json::Value &parsed, const std::string &cacheFile = "",
                                   unsigned int cacheExpiry = 0);

        virtual SError WatchdogGetEvents(int curPlayType, int eventActiveId, Json::Value &parsed);

    protected:
        virtual SError StalkerCall(sc_param_params_t *params, Json::Value &parsed, const std::string &cacheFile = "",
                                   unsigned int cacheExpiry = 0);

    private:
        sc_identity_t *m_identity;
        std::string m_endpoint;
        std::string m_basePath;
        std::string m_referer;
        unsigned int m_timeout;
    };
}
