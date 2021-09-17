/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace Base
{
struct Channel
{
  unsigned int uniqueId;
  int number;
  std::string name;
};

template<class ChannelType>
class ChannelManager
{
public:
  typedef typename std::vector<ChannelType>::iterator ChannelIterator;

  ChannelManager() = default;

  virtual ~ChannelManager() { m_channels.clear(); }

  virtual ChannelIterator GetChannelIterator(unsigned int uniqueId)
  {
    ChannelIterator it;
    it = std::find_if(m_channels.begin(), m_channels.end(),
                      [uniqueId](const Channel& c) { return c.uniqueId == uniqueId; });
    return it;
  }

  virtual ChannelType* GetChannel(unsigned int uniqueId)
  {
    ChannelIterator it = GetChannelIterator(uniqueId);
    return it != m_channels.end() ? &(*it) : nullptr;
  }

  virtual std::vector<ChannelType> GetChannels() { return m_channels; }

protected:
  std::vector<ChannelType> m_channels;
};
} // namespace Base
