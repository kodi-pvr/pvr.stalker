/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Utils.h"

#include "libstalkerclient/itv.h"

#include <iomanip>
#include <kodi/General.h>

std::string Utils::GetFilePath(const std::string& path, bool isUserPath)
{
  return (isUserPath ? kodi::addon::GetUserPath(path) : kodi::addon::GetAddonPath(path));
}

// http://stackoverflow.com/a/17708801
std::string Utils::UrlEncode(const std::string& value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
  {
    std::string::value_type c = (*i);

    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int((unsigned char)c);
  }

  return escaped.str();
}

int Utils::GetIntFromJsonValue(Json::Value& value, int defaultValue)
{
  int res = defaultValue;

  // some json responses have ints formated as strings
  if (value.isString())
    res = std::stoi(value.asString());
  else if (value.isInt())
    res = value.asInt();

  return res;
}

double Utils::GetDoubleFromJsonValue(Json::Value& value, double defaultValue)
{
  double res = defaultValue;

  /* some json responses have doubles formated as strings,
    or an expected double is formated as an int */
  if (value.isString())
    res = std::stod(value.asString());
  else if (value.isInt() || value.isDouble())
    res = value.asDouble();

  return res;
}

bool Utils::GetBoolFromJsonValue(Json::Value& value)
{
  // some json responses have string bools formated as string literals
  if (value.isString())
  {
    return value.asString().compare("true") == 0;
  }
  else
  {
    return value.asBool();
  }
}

std::string Utils::DetermineLogoURI(const std::string& basePath, const std::string& logo)
{
  std::string uri;

  if (logo.length() > 5 && logo.substr(0, 5).compare("data:") == 0)
  {
    return uri;
  }
  else if (logo.find("://") != std::string::npos)
  {
    uri = logo;
  }
  else if (logo.length() != 0)
  {
    uri = basePath + SC_ITV_LOGO_PATH_320 + logo;
  }

  return uri;
}
