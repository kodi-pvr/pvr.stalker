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

#include <ctime>
#include <map>
#include <string>
#include <vector>

#include "xbmc_epg_types.h"

#include "libstalkerclient/xmltv.h"
#include "HTTPSocket.h"

class XMLTV {
public:
    struct Credit {
        sc_xmltv_credit_type_t type;
        std::string name;
    };

    struct ProgrammeExtra {
        unsigned int broadcastId;
        std::string cast;
        std::string directors;
        std::string writers;
        int genreType;
        std::string genreDescription;
    };

    struct Programme {
        time_t start;
        time_t stop;
        std::string title;
        std::string subTitle;
        std::string desc;
        std::vector<Credit> credits;
        std::string date;
        std::vector<std::string> categories;
        int episodeNumber;
        time_t previouslyShown;
        std::string starRating;
        std::string icon;
        ProgrammeExtra extra;
    };

    struct Channel {
        std::string id;
        std::vector<std::string> displayNames;
        std::vector<Programme> programmes;
    };

    XMLTV();

    virtual ~XMLTV();

    virtual bool Parse(HTTPSocket::Scope scope, const std::string &path);

    virtual void Clear();

    virtual Channel *GetChannelById(const std::string &id);

    virtual Channel *GetChannelByDisplayName(std::string &displayName);

    virtual int EPGGenreByCategory(std::vector<std::string> &categories);

    virtual void SetUseCache(bool useCache) {
        m_useCache = useCache;
    }

    virtual void SetCacheFile(std::string cacheFile) {
        m_cacheFile = cacheFile;
    }

    virtual void SetCacheExpiry(unsigned int cacheExpiry) {
        m_cacheExpiry = cacheExpiry;
    }

protected:
    static std::vector<Credit> FilterCredits(std::vector<Credit> &credits, std::vector<sc_xmltv_credit_type_t> &types);

    static std::string CreditsAsString(std::vector<Credit> &credits, std::vector<sc_xmltv_credit_type_t> &types);

    static void AddCredit(std::vector<Credit> &credits, sc_xmltv_credit_type_t type, const char *name) {
        if (!name) return;

        Credit credit;
        credit.type = type;
        credit.name = name;

        credits.push_back(credit);
    }

    static std::map<int, std::vector<std::string>> CreateGenreMap() {
        std::map<int, std::vector<std::string>> genreMap;
        genreMap[EPG_EVENT_CONTENTMASK_UNDEFINED] = {"other"};
        genreMap[EPG_EVENT_CONTENTMASK_MOVIEDRAMA] = {"film", "movie", "movies"};
        genreMap[EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS] = {"news"};
        genreMap[EPG_EVENT_CONTENTMASK_SHOW] = {"episodic", "reality tv", "shows", "sitcoms", "talk show", "series"};
        genreMap[EPG_EVENT_CONTENTMASK_SPORTS] = {"football, golf, sports"};
        genreMap[EPG_EVENT_CONTENTMASK_CHILDRENYOUTH] = {"animation", "children", "kids", "under 5"};
        genreMap[EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE] = {};
        genreMap[EPG_EVENT_CONTENTMASK_ARTSCULTURE] = {};
        genreMap[EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS] = {};
        genreMap[EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE] = {"documentary", "educational", "science"};
        genreMap[EPG_EVENT_CONTENTMASK_LEISUREHOBBIES] = {"interests"};
        genreMap[EPG_EVENT_CONTENTMASK_SPECIAL] = {};

        return genreMap;
    }

private:
    bool m_useCache;
    std::string m_cacheFile;
    unsigned int m_cacheExpiry;
    std::vector<Channel> m_channels;
    std::map<int, std::vector<std::string>> m_genreMap;
};
