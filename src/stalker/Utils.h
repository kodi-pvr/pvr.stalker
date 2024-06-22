/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <json/json.h>
#include <sstream>
#include <string>

class Utils
{
public:
  static std::string GetFilePath(const std::string& path, bool isUserPath = true);

  static std::string UrlEncode(const std::string& string);

  static int GetIntFromJsonValue(Json::Value& value, int defaultValue = 0);

  static double GetDoubleFromJsonValue(Json::Value& value, double defaultValue = 0);

  static bool GetBoolFromJsonValue(Json::Value& value);

  static std::string DetermineLogoURI(const std::string& basePath, const std::string& logo);
};
