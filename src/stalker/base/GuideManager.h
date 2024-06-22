/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace Base
{
struct Event
{
  unsigned int uniqueBroadcastId;
  std::string title;
  unsigned int channelNumber;
  time_t startTime;
  time_t endTime;
};

template<class EventType>
class GuideManager
{
public:
  GuideManager() = default;
  virtual ~GuideManager() = default;
};
} // namespace Base
