/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

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
