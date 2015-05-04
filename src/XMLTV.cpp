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

//#include <algorithm>
#include <string>

#include "kodi/util/StringUtils.h"

#include "client.h"

using namespace ADDON;

XMLTV::XMLTV() {
  bLoaded = false;
  m_genreMap = XMLTV::CreateGenreMap();
}

XMLTV::~XMLTV() {
  m_channels.clear();
}

bool XMLTV::ReadFile(std::string &strPath)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  void *hdl;
  char buffer[1024];
  
  m_strXmlDoc.clear();
  
  hdl = XBMC->OpenFile(strPath.c_str(), 0);
  if (hdl) {
    memset(buffer, 0, sizeof(buffer));
    while (XBMC->ReadFileString(hdl, buffer, sizeof(buffer) - 1)) {
      m_strXmlDoc.append(buffer);
      memset(buffer, 0, sizeof(buffer));
    }
    
    XBMC->CloseFile(hdl);
  }
  
  return true;
}

bool XMLTV::ReadChannels(TiXmlElement *elemRoot)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  TiXmlElement *element;
  
  for (element = elemRoot->FirstChildElement("channel"); element != NULL;
    element = element->NextSiblingElement("channel"))
  {
    Channel chan;
    TiXmlElement *elem;
    
    chan.strId = element->Attribute("id");
    
    for (elem = element->FirstChildElement("display-name"); elem != NULL;
      elem = elem->NextSiblingElement("display-name"))
    {
      if (elem->GetText())
        chan.displayNames.push_back(elem->GetText());
    }
    
    m_channels.push_back(chan);
    
    XBMC->Log(LOG_DEBUG, "%s: id=%s | displayName=%s",
      __FUNCTION__, chan.strId.c_str(),
      !chan.displayNames.empty() ? chan.displayNames.front().c_str() : "");
  }
  
  return true;
}

bool XMLTV::ReadCredits(TiXmlElement *elemRoot, Programme *programme)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  TiXmlElement *element;

  for (element = elemRoot->FirstChildElement("actor"); element != NULL;
    element = element->NextSiblingElement("actor"))
      AddCredit(programme->credits, ACTOR, element->GetText());
  
  for (element = elemRoot->FirstChildElement("director"); element != NULL;
    element = element->NextSiblingElement("director"))
      AddCredit(programme->credits, DIRECTOR, element->GetText());
  
  for (element = elemRoot->FirstChildElement("guest"); element != NULL;
    element = element->NextSiblingElement("guest"))
      AddCredit(programme->credits, GUEST, element->GetText());

  for (element = elemRoot->FirstChildElement("presenter"); element != NULL;
    element = element->NextSiblingElement("presenter"))
      AddCredit(programme->credits, PRESENTER, element->GetText());

  for (element = elemRoot->FirstChildElement("producer"); element != NULL;
    element = element->NextSiblingElement("producer"))
      AddCredit(programme->credits, PRODUCER, element->GetText());

  for (element = elemRoot->FirstChildElement("writer"); element != NULL;
    element = element->NextSiblingElement("writer"))
      AddCredit(programme->credits, WRITER, element->GetText());
  
  return true;
}

bool XMLTV::ReadProgrammes(TiXmlElement *elemRoot)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  TiXmlElement *element;
  int iBroadcastId = 0;
  
  for (element = elemRoot->FirstChildElement("programme"); element != NULL;
    element = element->NextSiblingElement("programme"))
  {
    std::string strChanId;
    Channel *chan = NULL;
    TiXmlElement *elem;
    Programme programme;
    
    strChanId = element->Attribute("channel");
    chan = GetChannelById(strChanId);
    if (!chan) {
      XBMC->Log(LOG_DEBUG, "%s: channel \"%s\" not found",
        __FUNCTION__, strChanId.c_str());
      continue;
    }
    
    programme.iBroadcastId = ++iBroadcastId;
    programme.start = XmlTvToUnixTime(element->Attribute("start"));
    programme.stop = XmlTvToUnixTime(element->Attribute("stop"));
    
    elem = element->FirstChildElement("title");
    if (elem && elem->GetText())
      programme.strTitle = elem->GetText();
    
    elem = element->FirstChildElement("sub-title");
    if (elem && elem->GetText())
      programme.strSubTitle = elem->GetText();
    
    elem = element->FirstChildElement("desc");
    if (elem && elem->GetText())
      programme.strDesc = elem->GetText();
    
    elem = element->FirstChildElement("credits");
    if (elem)
      ReadCredits(elem, &programme);
    
    elem = element->FirstChildElement("date");
    if (elem && elem->GetText())
      programme.strDate = elem->GetText();
    
    for (elem = element->FirstChildElement("category"); elem != NULL;
      elem = elem->NextSiblingElement("category"))
    {
      if (elem->GetText())
        programme.categories.push_back(elem->GetText());
    }
    
    elem = element->FirstChildElement("episode-num");
    if (elem) {
      std::string strEpisodeNum;
      
      strEpisodeNum = elem->Attribute("system");
      if (strEpisodeNum.compare("onscreen") == 0) {
        //TODO move to utils
        int value = 0;
        try {
          std::istringstream iss(strEpisodeNum);
          iss >> value;
        }
        catch (...) {

        }
        programme.iEpisodeNumber = value;
      }
    }
    
    elem = element->FirstChildElement("previously-shown");
    if (elem)
      programme.previouslyShown = XmlTvToUnixTime(elem->Attribute("start"));
    
    elem = element->FirstChildElement("star-rating");
    if (elem) {
      elem = elem->FirstChildElement("value");
      if (elem && elem->GetText())
        programme.strStarRating = elem->GetText();
    }
    
    chan->programmes.push_back(programme);
    
    XBMC->Log(LOG_DEBUG, "%s: channel_id=%s | programme_title=%s",
      __FUNCTION__, chan->strId.c_str(), programme.strTitle.c_str());
  }
  
  return true;
}

bool XMLTV::ParseXML()
{
  TiXmlDocument doc;
  TiXmlElement *elemRoot;
  
  doc.Parse(m_strXmlDoc.c_str());
  if (doc.Error()) {
    XBMC->Log(LOG_ERROR, "%s: failed to load XMLTV data", __FUNCTION__);
    return false;
  }
  
  m_channels.clear();
  
  elemRoot = doc.FirstChildElement("tv");
  if (!elemRoot) {
    XBMC->Log(LOG_ERROR, "%s: root \"tv\" element not found", __FUNCTION__);
    return false;
  }
  
  if (!ReadChannels(elemRoot) || !ReadProgrammes(elemRoot)) {
    return false;
  }
  
  return true;
}

bool XMLTV::Parse(std::string &strPath)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  
  bLoaded = false;
  
  if (!ReadFile(strPath)) {
    return false;
  }
  
  if (!ParseXML()) {
    return false;
  }
  
  bLoaded = true;
  
  return true;
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
  std::map<int, int>::iterator finalMatch;
  
  for (std::map<int, std::string>::iterator genre = m_genreMap.begin(); genre != m_genreMap.end(); ++genre) {
    for (std::vector<std::string>::iterator category = categories.begin(); category != categories.end(); ++category) {
      if (genre->second.find(*category) != std::string::npos) {
        std::map<int, int>::iterator match = matches.find(genre->first);
        matches[genre->first] = match != matches.end() ? match->second + 1 : 1;
      }
    }
  }
  
  if (matches.empty())
    return EPG_GENRE_USE_STRING;
  
  finalMatch = matches.begin();
  for (std::map<int, int>::iterator match = matches.begin(); match != matches.end(); ++match) {
    if (match->second > finalMatch->second)
      finalMatch = match;
  }
  
  return finalMatch->first;
}
