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

#include "XMLTV.h"

#include <algorithm>
#include <string>
#include <vector>

#include "p8-platform/util/StringUtils.h"

#include "libstalkerclient/list.h"
#include "libstalkerclient/xmltv.h"
#include "client.h"
#include "Utils.h"

using namespace ADDON;

XMLTV::XMLTV() {
  m_genreMap = XMLTV::CreateGenreMap();
}

XMLTV::~XMLTV() {
  m_channels.clear();
}

bool XMLTV::Parse(Scope scope, std::string &strPath, bool bCache, uint32_t cacheExpiry)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  HTTPSocket sock(g_iConnectionTimeout);
  Request request;
  Response response;
  sc_list_t *xmltv_channels = NULL;

  m_channels.clear();

  request.scope = scope;
  request.url = strPath;

  response.useCache = true;
  response.url = Utils::GetFilePath("epg_xmltv.xml");
  response.expiry = cacheExpiry;
  response.writeToBody = false;

  if (!sock.Execute(request, response) || !(xmltv_channels = sc_xmltv_parse(response.url.c_str()))) {
    XBMC->Log(LOG_ERROR, "%s: failed to load XMLTV data", __FUNCTION__);
  }

  if ((!xmltv_channels || !bCache) && XBMC->FileExists(response.url.c_str(), false)) {
#ifdef TARGET_WINDOWS
    DeleteFile(response.url.c_str());
#else
    XBMC->DeleteFile(response.url.c_str());
#endif
  }

  if (!xmltv_channels)
    return false;

  sc_list_node_t *node1 = xmltv_channels->first;
  while (node1) {
    sc_xmltv_channel_t *chan = (sc_xmltv_channel_t *)node1->data;

    Channel c;
    if (chan->id_) c.strId = chan->id_;

    sc_list_node_t *node2 = chan->display_names->first;
    while (node2) {
      if (node2->data)
        c.displayNames.push_back((const char *)node2->data);

      XBMC->Log(LOG_DEBUG, "%s", c.displayNames.back().c_str());

      node2 = node2->next;
    }

    node2 = chan->programmes->first;
    int iBroadcastId = 0;
    while (node2) {
      sc_xmltv_programme_t *prog = (sc_xmltv_programme_t *)node2->data;

      Programme p;
      p.iBroadcastId = ++iBroadcastId;
      p.start = prog->start;
      p.stop = prog->stop;
      if (prog->title) p.strTitle = prog->title;
      if (prog->sub_title) p.strSubTitle = prog->sub_title;
      if (prog->desc) p.strDesc = prog->desc;

      sc_list_node_t *node3 = prog->credits->first;
      while (node3) {
        sc_xmltv_credit_t *cred = (sc_xmltv_credit_t *)node3->data;

        AddCredit(p.credits, (CreditType)cred->type, (const char *)cred->name);

        node3 = node3->next;
      }
      std::vector<Credit> cast;
      std::vector<Credit> cast2;
      cast = XMLTV::FilterCredits(p.credits, ACTOR);
      Utils::ConcatenateVectors(cast, (cast2 = XMLTV::FilterCredits(p.credits, GUEST)));
      Utils::ConcatenateVectors(cast, (cast2 = XMLTV::FilterCredits(p.credits, PRESENTER)));
      p.strCast = Utils::ConcatenateStringList(XMLTV::StringListForCreditType(cast));
      p.strDirectors = Utils::ConcatenateStringList(XMLTV::StringListForCreditType(p.credits, DIRECTOR));
      p.strWriters = Utils::ConcatenateStringList(XMLTV::StringListForCreditType(p.credits, WRITER));

      if (prog->date) p.strDate = prog->date;

      node3 = prog->categories->first;
      while (node3) {
        if (node3->data)
          p.categories.push_back((const char *)node3->data);

        node3 = node3->next;
      }
      p.strCategories = Utils::ConcatenateStringList(p.categories);

      p.iEpisodeNumber = prog->episode_num;
      p.previouslyShown = prog->previously_shown;
      if (prog->star_rating) p.strStarRating = prog->star_rating;
      if (prog->icon) p.strIcon = prog->icon;

      c.programmes.push_back(p);

      prog = NULL;
      node2 = node2->next;
    }

    m_channels.push_back(c);

    chan = NULL;
    node1 = node1->next;
  }

  sc_xmltv_list_free(SC_XMLTV_CHANNEL, &xmltv_channels);

  return true;
}

void XMLTV::Clear()
{
  m_channels.clear();
}

Channel* XMLTV::GetChannelById(std::string &strId)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  Channel *chan = NULL;

  for (std::vector<Channel>::iterator it = m_channels.begin(); it != m_channels.end(); ++it) {
    if (it->strId.compare(strId) == 0) {
      chan = &(*it);
      break;
    }
  }

  return chan;
}

Channel* XMLTV::GetChannelByDisplayName(std::string &strDisplayName)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  std::vector<std::string>::iterator displayName;
  std::string strTemp;
  Channel *chan = NULL;

  for (std::vector<Channel>::iterator it = m_channels.begin(); it != m_channels.end(); ++it) {
    displayName = std::find(it->displayNames.begin(), it->displayNames.end(), strDisplayName);

    // try compensating for already unescaped chars that TinyXML ignores
    if (displayName == it->displayNames.end()) {
      strTemp = strDisplayName; StringUtils::Replace(strTemp, "&", "");
      displayName = std::find(it->displayNames.begin(), it->displayNames.end(), strTemp);
    }

    if (displayName != it->displayNames.end()) {
      chan = &(*it);
      break;
    }
  }

  return chan;
}

int XMLTV::EPGGenreByCategory(std::vector<std::string> &categories)
{
  std::map<int, int> matches;
  std::map<int, int>::iterator finalMatch = matches.end();

  for (std::vector<std::string>::iterator category = categories.begin(); category != categories.end(); ++category) {
    for (std::map<int, std::string>::iterator genre = m_genreMap.begin(); genre != m_genreMap.end(); ++genre) {
      std::string cat = *category; StringUtils::ToLower(cat);
      std::string gen = genre->second; StringUtils::ToLower(gen);

      if (gen.find(cat) != std::string::npos) {
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

  for (std::map<int, int>::iterator match = matches.begin(); match != matches.end(); ++match) {
    if (match->second > finalMatch->second)
      finalMatch = match;
  }

  return finalMatch->first;
}

std::vector<Credit> XMLTV::FilterCredits(std::vector<Credit> &credits, CreditType type)
{
  std::vector<Credit> filteredCredits;

  for (std::vector<Credit>::iterator credit = credits.begin(); credit != credits.end(); ++credit) {
    if (credit->type == type)
      filteredCredits.push_back(*credit);
  }

  return filteredCredits;
}

std::vector<std::string> XMLTV::StringListForCreditType(std::vector<Credit> &credits, CreditType type)
{
  std::vector<Credit> filteredCredits;
  std::vector<std::string> creditList;

  filteredCredits = type != ALL
    ? FilterCredits(credits, type)
    : credits;

  for (std::vector<Credit>::iterator credit = filteredCredits.begin(); credit != filteredCredits.end(); ++credit)
    creditList.push_back(credit->strName);

  return creditList;
}
