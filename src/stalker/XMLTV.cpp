/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015, 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "XMLTV.h"

#include "Utils.h"

#include <algorithm>
#include <iterator>
#include <kodi/Filesystem.h>
#include <kodi/tools/StringUtils.h>

XMLTV::~XMLTV()
{
  Clear();
}

bool XMLTV::Parse(HTTPSocket::Scope scope, const std::string& path)
{
  kodi::Log(ADDON_LOG_DEBUG, "%s", __func__);

  HTTPSocket sock(15);
  HTTPSocket::Request request;
  HTTPSocket::Response response;
  sc_list_t* xmltv_channels = nullptr;

  Clear();

  request.scope = scope;
  request.url = path;

  response.useCache = m_useCache;
  response.url = m_cacheFile;
  response.expiry = m_cacheExpiry;
  response.writeToBody = false;

  if (!sock.Execute(request, response) || !(xmltv_channels = sc_xmltv_parse(m_cacheFile.c_str())))
    kodi::Log(ADDON_LOG_ERROR, "%s: failed to load XMLTV data", __func__);

  if ((!xmltv_channels || !m_useCache) && kodi::vfs::FileExists(m_cacheFile, false))
  {
    kodi::vfs::DeleteFile(response.url);
  }

  if (!xmltv_channels)
    return false;

  unsigned int broadcastId(0);
  std::vector<sc_xmltv_credit_type_t> cast = {
      SC_XMLTV_CREDIT_TYPE_ACTOR, SC_XMLTV_CREDIT_TYPE_GUEST, SC_XMLTV_CREDIT_TYPE_PRESENTER};
  std::vector<sc_xmltv_credit_type_t> directors = {SC_XMLTV_CREDIT_TYPE_DIRECTOR};
  std::vector<sc_xmltv_credit_type_t> writers = {SC_XMLTV_CREDIT_TYPE_WRITER};

  sc_list_node_t* node1 = xmltv_channels->first;
  while (node1)
  {
    sc_xmltv_channel_t* chan = (sc_xmltv_channel_t*)node1->data;

    Channel c;
    if (chan->id_)
      c.id = chan->id_;

    sc_list_node_t* node2 = chan->display_names->first;
    while (node2)
    {
      if (node2->data)
        c.displayNames.push_back((const char*)node2->data);

      node2 = node2->next;
    }

    if (c.displayNames.size())
      kodi::Log(ADDON_LOG_DEBUG, "%s", c.displayNames.front().c_str());

    node2 = chan->programmes->first;
    while (node2)
    {
      sc_xmltv_programme_t* prog = (sc_xmltv_programme_t*)node2->data;

      Programme p;
      p.extra.broadcastId = ++broadcastId;
      p.start = prog->start;
      p.stop = prog->stop;
      if (prog->title)
        p.title = prog->title;
      if (prog->sub_title)
        p.subTitle = prog->sub_title;
      if (prog->desc)
        p.desc = prog->desc;

      sc_list_node_t* node3 = prog->credits->first;
      while (node3)
      {
        sc_xmltv_credit_t* cred = (sc_xmltv_credit_t*)node3->data;

        AddCredit(p.credits, cred->type, (const char*)cred->name);

        node3 = node3->next;
      }
      p.extra.cast = CreditsAsString(p.credits, cast);
      p.extra.directors = CreditsAsString(p.credits, directors);
      p.extra.writers = CreditsAsString(p.credits, writers);

      if (prog->date)
        p.date = prog->date;

      node3 = prog->categories->first;
      while (node3)
      {
        if (node3->data)
          p.categories.push_back((const char*)node3->data);

        node3 = node3->next;
      }
      p.extra.genreType = EPGGenreByCategory(p.categories);
      p.extra.genreDescription = kodi::tools::StringUtils::Join(p.categories, ", ");

      p.episodeNumber = prog->episode_num;
      p.previouslyShown = prog->previously_shown;
      if (prog->star_rating)
        p.starRating = prog->star_rating;
      if (prog->icon)
        p.icon = prog->icon;

      c.programmes.push_back(p);

      node2 = node2->next;
    }

    m_channels.push_back(c);

    node1 = node1->next;
  }

  sc_xmltv_list_free(SC_XMLTV_CHANNEL, &xmltv_channels);

  return true;
}

void XMLTV::Clear()
{
  m_channels.clear();
}

XMLTV::Channel* XMLTV::GetChannelById(const std::string& id)
{
  std::vector<Channel>::iterator it;
  Channel* chan = nullptr;

  it = std::find_if(m_channels.begin(), m_channels.end(),
                    [id](const Channel& channel) { return !id.compare(channel.id); });

  if (it != m_channels.end())
    chan = &(*it);

  return chan;
}

XMLTV::Channel* XMLTV::GetChannelByDisplayName(std::string& displayName)
{
  std::vector<Channel>::iterator it;
  Channel* chan = nullptr;

  it = std::find_if(m_channels.begin(), m_channels.end(), [displayName](const Channel& channel) {
    std::vector<std::string>::const_iterator dnIt;

    dnIt = std::find_if(channel.displayNames.begin(), channel.displayNames.end(),
                        [displayName](const std::string& dn) {
                          return !kodi::tools::StringUtils::CompareNoCase(displayName, dn);
                        });

    return dnIt != channel.displayNames.end();
  });

  if (it != m_channels.end())
    chan = &(*it);

  return chan;
}

int XMLTV::EPGGenreByCategory(std::vector<std::string>& categories)
{
  std::map<int, int> matches;
  std::map<int, int>::iterator finalMatch = matches.end();

  for (std::vector<std::string>::iterator category = categories.begin();
       category != categories.end(); ++category)
  {
    for (std::map<int, std::vector<std::string>>::iterator genre = m_genreMap.begin();
         genre != m_genreMap.end(); ++genre)
    {
      std::vector<std::string> genreCategories = genre->second;
      std::vector<std::string>::iterator gmIt;

      gmIt = std::find_if(
          genreCategories.begin(), genreCategories.end(),
          [category](const std::string& g) { return !kodi::tools::StringUtils::CompareNoCase(*category, g); });

      if (gmIt != genreCategories.end())
      {
        // find the genre match count, if found previously
        std::map<int, int>::iterator match = matches.find(genre->first);
        // increment the number of matches for the genre
        matches[genre->first] = match != matches.end() ? match->second + 1 : 1;
        // set final match to the first match
        // in the case that no dominant genre set is found this will be used
        if (finalMatch == matches.end())
          finalMatch = matches.find(genre->first);
      }
    }
  }

  if (matches.empty() || finalMatch == matches.end())
    return EPG_GENRE_USE_STRING;

  for (std::map<int, int>::iterator match = matches.begin(); match != matches.end(); ++match)
  {
    if (match->second > finalMatch->second)
      finalMatch = match;
  }

  return finalMatch->first;
}

std::vector<XMLTV::Credit> XMLTV::FilterCredits(std::vector<Credit>& credits,
                                                std::vector<sc_xmltv_credit_type_t>& types)
{
  std::vector<Credit> filteredCredits;

  std::copy_if(credits.begin(), credits.end(), std::back_inserter(filteredCredits),
               [types](const Credit& credit) {
                 std::vector<sc_xmltv_credit_type_t>::const_iterator ctIt;

                 ctIt = std::find(types.begin(), types.end(), credit.type);

                 return ctIt != types.end();
               });

  return filteredCredits;
}

std::string XMLTV::CreditsAsString(std::vector<Credit>& credits,
                                   std::vector<sc_xmltv_credit_type_t>& types)
{
  std::vector<Credit> filteredCredits;
  std::vector<std::string> creditList;

  filteredCredits = FilterCredits(credits, types);

  for (std::vector<Credit>::iterator credit = filteredCredits.begin();
       credit != filteredCredits.end(); ++credit)
    creditList.push_back(credit->name);

  return kodi::tools::StringUtils::Join(creditList, ", ");
}
