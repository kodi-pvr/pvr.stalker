/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Error.h"
#include "SAPI.h"
#include "base/ChannelManager.h"

namespace SC
{
struct Channel : Base::Channel
{
  std::string streamUrl;
  std::string iconPath;
  int channelId;
  std::string cmd;
  std::string tvGenreId;
  bool useHttpTmpLink;
  bool useLoadBalancing;
};

struct ChannelGroup
{
  std::string id;
  std::string name;
  std::string alias;
};

class ChannelManager : public Base::ChannelManager<Channel>
{
public:
  ChannelManager() = default;
  virtual ~ChannelManager() = default;

  virtual void SetAPI(SAPI* api) { m_api = api; }

  virtual SError LoadChannels();

  virtual SError LoadChannelGroups();

  virtual ChannelGroup* GetChannelGroup(const std::string& name)
  {
    std::vector<ChannelGroup>::iterator cgIt;
    cgIt = std::find_if(m_channelGroups.begin(), m_channelGroups.end(),
                        [&name](const ChannelGroup& cg) { return !cg.name.compare(name); });
    return cgIt != m_channelGroups.end() ? &(*cgIt) : nullptr;
  }

  virtual std::vector<ChannelGroup> GetChannelGroups() { return m_channelGroups; }

  virtual std::string GetStreamURL(Channel& channel);

private:
  bool ParseChannels(Json::Value& parsed);

  //https://github.com/kodi-pvr/pvr.iptvsimple/blob/master/src/PVRIptvData.cpp#L1085
  int GetChannelId(const char* strChannelName, const char* strStreamUrl);

  bool ParseChannelGroups(Json::Value& parsed);

  std::string ParseStreamCmd(Json::Value& parsed);

  SAPI* m_api = nullptr;
  std::vector<ChannelGroup> m_channelGroups;
};
} // namespace SC
