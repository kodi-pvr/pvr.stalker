/*
 *      Copyright (C) 2015  Jamal Edey
 *      http://www.kenshisoft.com/
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

#include "Utils.h"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

// http://stackoverflow.com/a/17708801
std::string Utils::UrlEncode(const std::string &value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
    std::string::value_type c = (*i);

    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int((unsigned char) c);
  }

  return escaped.str();
}

double Utils::StringToDouble(const std::string &value)
{
  std::istringstream iss(value);
  double result;
  
  iss >> result;
  
  return result;
}

int Utils::StringToInt(const std::string &value)
{
  return (int)StringToDouble(value);
}

// http://stackoverflow.com/a/8581865
std::string Utils::ConcatenateStringList(const std::vector<std::string> &list)
{
  std::ostringstream oss;
  
  if (!list.empty()) {
    std::copy(list.begin(), list.end() - 1, 
      std::ostream_iterator<std::string>(oss, ", "));
    
    oss << list.back();
  }
  
  return oss.str();
}

int Utils::GetIntFromJsonValue(Json::Value &value, int defaultValue)
{
  int iTemp = defaultValue;

  // some json responses have have ints formated as strings
  if (value.isString())
    iTemp = StringToInt(value.asString());
  else if (value.isInt())
    iTemp = value.asInt();

  return iTemp;
}

double Utils::GetDoubleFromJsonValue(Json::Value &value, double defaultValue)
{
  double dTemp = defaultValue;

  // some json responses have have doubles formated as strings, \
  or an expected double is formated as an int
  if (value.isString())
    dTemp = StringToDouble(value.asString());
  else if (value.isInt() || value.isDouble())
    dTemp = value.asDouble();

  return dTemp;
}
