#pragma once

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

#include <string>
#include <vector>
#include <ctime>
#include <map>

#include "tinyxml.h"
#include "kodi/xbmc_epg_types.h"

typedef enum CreditType {
  ACTOR,
  DIRECTOR,
  GUEST,
  PRESENTER,
  PRODUCER,
  WRITER
};

struct Credit
{
  CreditType  type;
  std::string strName;
};

struct Programme
{
  int                       iBroadcastId;
  time_t                    start;
  time_t                    stop;
  std::string               strTitle;
  std::string               strSubTitle;
  std::string               strDesc;
  std::vector<Credit>       credits;
  std::string               strDate;
  std::vector<std::string>  categories;
  int                       iEpisodeNumber;
  time_t                    previouslyShown;
  std::string               strStarRating;
};

struct Channel
{
  std::string               strId;
  std::vector<std::string>  displayNames;
  std::vector<Programme>    programmes;
};

class XMLTV
{
public:
  XMLTV();
  virtual ~XMLTV();
  
  virtual bool Parse(std::string &strPath);
  virtual Channel* GetChannelById(std::string &strId);
  virtual Channel* GetChannelByDisplayName(std::string &strDisplayName);
  virtual int EPGGenreByCategory(std::vector<std::string> &categories);
  
  bool bLoaded;
protected:
  virtual bool ReadFile(std::string &strPath); //TODO sync with httpsocket from http branch
  
  static time_t XmlTvToUnixTime(const char *strTime)
  {
    if (!strTime)
      return 0;
    
    std::tm timeinfo;
    int iHourOffset = 0;
    time_t time;
    
    sscanf(strTime, "%04d%02d%02d%02d%02d%02d",
      &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
      &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);

    timeinfo.tm_year -= 1900;
    timeinfo.tm_mon -= 1;
    timeinfo.tm_isdst = -1;
    
    if (strlen(strTime) == 20) {
      sscanf(&strTime[16], "%02d", &iHourOffset);
      iHourOffset *= 3600;
    }
    
    time = mktime(&timeinfo);
    time += timeinfo.tm_isdst != 0 ? 3600 : -3600;
    time += iHourOffset - timezone;

    return time;
  }
  
  static void AddCredit(std::vector<Credit> &credits, CreditType type, const char *name)
  {
    if (!name)
      return;
    
    Credit credit;
    credit.type = type;
    credit.strName = name;
    
    credits.push_back(credit);
  }
  
  static std::map<int, std::string> CreateGenreMap()
  {
    std::map<int, std::string> genreMap;
    genreMap[EPG_EVENT_CONTENTMASK_UNDEFINED] = "Other";
    genreMap[EPG_EVENT_CONTENTMASK_MOVIEDRAMA] = "Film, Movie, Movies";
    genreMap[EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS] = "News";
    genreMap[EPG_EVENT_CONTENTMASK_SHOW] = "Episodic, Reality TV, Shows, Sitcoms, Talk Show";//, Series";
    genreMap[EPG_EVENT_CONTENTMASK_SPORTS] = "Football, Golf, Sports";
    genreMap[EPG_EVENT_CONTENTMASK_CHILDRENYOUTH] = "Animation, Children, Kids, Under 5";
    genreMap[EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE] = "";
    genreMap[EPG_EVENT_CONTENTMASK_ARTSCULTURE] = "";
    genreMap[EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS] = "";
    genreMap[EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE] = "Documentary, Educational, Science";
    genreMap[EPG_EVENT_CONTENTMASK_LEISUREHOBBIES] = "Interests";
    genreMap[EPG_EVENT_CONTENTMASK_SPECIAL] = "";
    
    return genreMap;
  }
  
  virtual bool ReadChannels(TiXmlElement *elemRoot);
  virtual bool ReadCredits(TiXmlElement *elemRoot, Programme *programme);
  virtual bool ReadProgrammes(TiXmlElement *elemRoot);
  virtual bool ParseXML();
private:
  std::string                 m_strXmlDoc;
  std::vector<Channel>        m_channels;
  std::map<int, std::string>  m_genreMap;
};
