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

#include <sstream>
#include <string>

#include <json/json.h>

class Utils {
public:
    static std::string GetFilePath(const std::string &path, bool isUserPath = true);

    static std::string UrlEncode(const std::string &string);

    static double StringToDouble(const std::string &value);

    static int StringToInt(const std::string &value);

    static int GetIntFromJsonValue(Json::Value &value, int defaultValue = 0);

    static double GetDoubleFromJsonValue(Json::Value &value, double defaultValue = 0);

    static bool GetBoolFromJsonValue(Json::Value &value);

    static std::string DetermineLogoURI(const std::string &basePath, const std::string &logo);

    template<typename T>
    static std::string ToString(const T &value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
};
