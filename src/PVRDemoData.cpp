/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "tinyxml/XMLUtils.h"
#include "PVRDemoData.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <regex>

using namespace std;
using namespace ADDON;

const static std::string m_authFailedStr = "Authorization failed.";
bool m_authFailed = false;

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

bool PVRDemoData::InitAPI()
{
	struct MemoryStruct chunk;
	CURL *curl_handle;
	CURLcode res;

	chunk.memory = (char *)malloc(1);
	chunk.size = 0;

	//curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, g_strServer.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&chunk);

	res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK) {
		XBMC->Log(LOG_ERROR, "InitAPI: failed: %s\n", curl_easy_strerror(res));
		free(chunk.memory);
		return false;
	}

	curl_easy_cleanup(curl_handle);

	//curl_global_cleanup();

	// xpcom.common.js > get_server_params

	string cmd = chunk.memory;
	free(chunk.memory);

	regex cmdRegex("Location:\\s(https?):\\/\\/(.+)\\/(.+)\\/(.+)\\/(.+)");
	smatch cmdMatch;
	if (!regex_search(cmd, cmdMatch, cmdRegex)) {
		XBMC->Log(LOG_ERROR, "failed to get api endpoint\n");
		return false;
	}

	m_apiEndpoint = cmdMatch[1].str() + "://" + cmdMatch[2].str() + "/" + cmdMatch[3].str() + "/server/load.php";
	m_referrer = cmdMatch[1].str() + "://" + cmdMatch[2].str() + "/" + cmdMatch[3].str() + "/" + cmdMatch[4].str() + "/";

	XBMC->Log(LOG_ERROR, "api endpoint: %s\n", m_apiEndpoint.c_str());
	XBMC->Log(LOG_ERROR, "referrer: %s\n", m_referrer.c_str());

	return true;
}

bool PVRDemoData::DoAPICall(string *url, struct MemoryStruct *chunk)
{
	string finalUrl;
	string cookie;
	string authHeader;
	CURL *curl_handle;
	CURLcode res;

	finalUrl = m_apiEndpoint + *url;

	int pos = 0;
	cookie = "mac=" + g_strMac + "; stb_lang=en; timezone=Europe%2FKiev";
	while ((pos = cookie.find(":")) > 0) {
		cookie.replace(pos, 1, "%3A");
	}

	authHeader = "Authorization: Bearer " + m_token;

	//curl_global_init(CURL_GLOBAL_ALL);

	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, finalUrl.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, cookie.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3");
	curl_easy_setopt(curl_handle, CURLOPT_REFERER, m_referrer.c_str());

	struct curl_slist *headers = NULL; // init to NULL is important
	headers = curl_slist_append(headers, "X-User-Agent: Model: MAG250; Link: WiFi");
	headers = curl_slist_append(headers, authHeader.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	res = curl_easy_perform(curl_handle);
	if (res != CURLE_OK) {
		XBMC->Log(LOG_ERROR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		return false;
	}
	else {
		XBMC->Log(LOG_ERROR, "%lu bytes retrieved\n", (long)chunk->size);
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl_handle);

	//curl_global_cleanup();

	return true;
}

bool PVRDemoData::Handshake()
{
	string url;
	struct MemoryStruct chunk;
	Json::Value parsedFromString;
	Json::Reader reader;
	bool parsingSuccessful;

	url = "?type=stb&action=handshake&JsHttpRequest=1-xml&";

	chunk.memory = (char *)malloc(1);
	chunk.size = 0;

	if (!DoAPICall(&url, &chunk)) {
		XBMC->Log(LOG_ERROR, "Handshake: api call failed\n");
		free(chunk.memory);
		return false;
	}

	parsingSuccessful = reader.parse(chunk.memory, parsedFromString);

	free(chunk.memory);

	if (!parsingSuccessful) {
		XBMC->Log(LOG_ERROR, "Handshake: parsing failed\n");
		return false;
	}

	m_token = parsedFromString["js"]["token"].asString();

	XBMC->Log(LOG_ERROR, "token: %s\n", m_token.c_str());

	return true;
}

bool PVRDemoData::GetProfile()
{
	string url;
	struct MemoryStruct chunk;
	Json::Value parsedFromString;
	Json::Reader reader;
	bool parsingSuccessful;

	url = "?type=stb&action=get_profile"
		"&hd=1&ver=ImageDescription:%200.2.16-250;%20ImageDate:%2018%20Mar%202013%2019:56:53%20GMT+0200;%20PORTAL%20version:%204.9.9;%20API%20Version:%20JS%20API%20version:%20328;%20STB%20API%20version:%20134;%20Player%20Engine%20version:%200x560"
		"&num_banks=1&sn=0000000000000&stb_type=MAG250&image_version=216&auth_second_step=0&hw_version=1.7-BD-00&not_valid_token=0&JsHttpRequest=1-xml&";

	chunk.memory = (char *)malloc(1);
	chunk.size = 0;

	if (!DoAPICall(&url, &chunk)) {
		XBMC->Log(LOG_ERROR, "GetProfile: api call failed\n");
		free(chunk.memory);
		return false;
	}

	parsingSuccessful = reader.parse(chunk.memory, parsedFromString);

	free(chunk.memory);

	if (!parsingSuccessful) {
		XBMC->Log(LOG_ERROR, "GetProfile: parsing failed\n");
		return false;
	}

	// do nothing

	return true;
}

bool PVRDemoData::GetAllChannels(Json::Value *parsed)
{
	string url;
	struct MemoryStruct chunk;
	Json::Reader reader;
	bool parsingSuccessful;

	url = "?type=itv&action=get_all_channels&JsHttpRequest=1-xml&";

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	if (!DoAPICall(&url, &chunk)) {
		XBMC->Log(LOG_ERROR, "GetAllChannels: api call failed\n");
		free(chunk.memory);
		return false;
	}

	parsingSuccessful = reader.parse(chunk.memory, *parsed);

	free(chunk.memory);

	if (!parsingSuccessful) {
		XBMC->Log(LOG_ERROR, "GetAllChannels: parsing failed\n");
		if (chunk.memory == m_authFailedStr) {
			m_authFailed = true;
		}
		return false;
	}

	return true;
}

bool PVRDemoData::Authenticate()
{
	XBMC->Log(LOG_ERROR, "authenticating\n");

	if (!Handshake() || !GetProfile()) {
		return false;
	}

	TiXmlDocument xmlDoc;
	string strSettingsFile = GetSettingsFile(g_strUserPath);

	if (!xmlDoc.LoadFile(strSettingsFile))
	{
		XBMC->Log(LOG_ERROR, "invalid demo data (no/invalid data file found at '%s')", strSettingsFile.c_str());
		return false;
	}

	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (strcmp(pRootElement->Value(), "cache") != 0)
	{
		XBMC->Log(LOG_ERROR, "invalid demo data (no <demo> tag found)");
		return false;
	}

	XBMC->Log(LOG_ERROR, "saving token\n");

	TiXmlElement *pSElement = pRootElement->FirstChildElement("token");
	pSElement->Clear();
	pSElement->LinkEndChild(new TiXmlText(m_token));

	if (!xmlDoc.SaveFile(strSettingsFile))
	{
		XBMC->Log(LOG_ERROR, "failed to save %s", strSettingsFile.c_str());
		return false;
	}

	return true;
}

bool PVRDemoData::LoadChannels()
{
	Json::Value parsed;
	int iUniqueChannelId = 0;

	if (!GetAllChannels(&parsed)) {
		XBMC->Log(LOG_ERROR, "LoadChannels: failed\n");
		return false;
	}

	for (Json::Value::iterator it = parsed["js"]["data"].begin(); it != parsed["js"]["data"].end(); ++it) {
		// try to get the stream url first
		string cmd = (*it)["cmd"].asString();
		regex cmdRegex("ffrt2\\s(.+)");
		smatch cmdMatch;
		if (!regex_match(cmd, cmdMatch, cmdRegex)) {
			XBMC->Log(LOG_ERROR, "no stream url. skipping\n");
			continue;
		}

		//ssub_match cmdSubMatch = cmdMatch[1];

		PVRDemoChannel channel;
		channel.iUniqueId = ++iUniqueChannelId;

		/* channel name */
		channel.strChannelName = (*it)["name"].asString();

		/* radio/TV */
		channel.bRadio = false;

		string number = (*it)["number"].asString();

		/* channel number */
		channel.iChannelNumber = std::stoi(number);

		/* sub channel number */
		channel.iSubChannelNumber = 0;

		/* CAID */
		channel.iEncryptionSystem = 0;

		/* icon path */
		channel.strIconPath = "";

		/* stream url */
		channel.strStreamURL = "pvr://stream/" + cmdMatch[1].str(); // GetLiveStreamURL
		//channel.strStreamURL = ""; // this will cause OpenLiveStream to be called 
		channel.strStreamURL2 = cmdMatch[1].str();

		m_channels.push_back(channel);

		XBMC->Log(LOG_ERROR, "%d - %s\n", channel.iChannelNumber, channel.strChannelName.c_str());
	}

	return true;
}

PVRDemoData::PVRDemoData(void)
{
  m_iEpgStart = -1;
  m_strDefaultIcon =  "http://www.royalty-free.tv/news/wp-content/uploads/2011/06/cc-logo1.jpg";
  m_strDefaultMovie = "";

  LoadDemoData();
}

PVRDemoData::~PVRDemoData(void)
{
	XBMC->Log(LOG_ERROR, "->~PVRDemoData()");

  m_channels.clear();
  m_groups.clear();
}

std::string PVRDemoData::GetSettingsFile(string settingFile) const
{
  //string settingFile = g_strClientPath;
  if (settingFile.at(settingFile.size() - 1) == '\\' ||
      settingFile.at(settingFile.size() - 1) == '/')
    settingFile.append("cache.xml");
  else
    settingFile.append("/cache.xml");
  return settingFile;
}

bool PVRDemoData::LoadDemoData(void)
{
  TiXmlDocument xmlDoc;
  string strUSettingsFile = GetSettingsFile(g_strUserPath);
  bool bUSettingsFileExists = XBMC->FileExists(strUSettingsFile.c_str(), false);
  string strSettingsFile = bUSettingsFileExists ? strUSettingsFile : GetSettingsFile(g_strClientPath);

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    XBMC->Log(LOG_ERROR, "invalid demo data (no/invalid data file found at '%s')", strSettingsFile.c_str());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmp(pRootElement->Value(), "cache") != 0)
  {
    XBMC->Log(LOG_ERROR, "invalid demo data (no <demo> tag found)");
    return false;
  }

  if (!bUSettingsFileExists) {
	  XBMC->Log(LOG_ERROR, "saving to user cache\n");
	  if (!xmlDoc.SaveFile(strUSettingsFile)) {
		  XBMC->Log(LOG_ERROR, "failed to save %s", strUSettingsFile.c_str());
		  return false;
	  }
  }

	/*TiXmlNode *pSNode = pRootElement->FirstChild("settings");
	CStdString strTmp;

	if (!XMLUtils::GetString(pSNode, "token", strTmp)) {
		m_token = "";
	}
	else {
		m_token = strTmp;
	}*/

  TiXmlElement *pSElement = pRootElement->FirstChildElement("token");
  if (!pSElement) {
	  XBMC->Log(LOG_ERROR, "token element not found\n");
  }
  else if (pSElement->GetText()) {
	  m_token = pSElement->GetText();
  }

  XBMC->Log(LOG_ERROR, "token: %s\n", m_token.c_str());

  if (!InitAPI()) {
	  XBMC->Log(LOG_ERROR, "failed to init api\n");
	  return false;
  }

  if (m_token.empty() && !Authenticate()) {
	  XBMC->Log(LOG_ERROR, "authentication failed\n");
	  return false;
  }

  if (!LoadChannels()) {
	  if (m_authFailed) {
		  XBMC->Log(LOG_ERROR, "re-authenticating\n");
		  if (!Authenticate()) {
			  XBMC->Log(LOG_ERROR, "authentication failed\n");
		  }
		  XBMC->Log(LOG_ERROR, "re-attempting to load channels\n");
		  LoadChannels();
	  }
  }

  return true;

  /* load channels */
  int iUniqueChannelId = 0;
  TiXmlElement *pElement = pRootElement->FirstChildElement("channels");
  if (pElement)
  {
    TiXmlNode *pChannelNode = NULL;
    while ((pChannelNode = pElement->IterateChildren(pChannelNode)) != NULL)
    {
      CStdString strTmp;
      PVRDemoChannel channel;
      channel.iUniqueId = ++iUniqueChannelId;

      /* channel name */
      if (!XMLUtils::GetString(pChannelNode, "name", strTmp))
        continue;
      channel.strChannelName = strTmp;

      /* radio/TV */
      XMLUtils::GetBoolean(pChannelNode, "radio", channel.bRadio);

      /* channel number */
      if (!XMLUtils::GetInt(pChannelNode, "number", channel.iChannelNumber))
        channel.iChannelNumber = iUniqueChannelId;

      /* sub channel number */
      if (!XMLUtils::GetInt(pChannelNode, "subnumber", channel.iSubChannelNumber))
        channel.iSubChannelNumber = 0;

      /* CAID */
      if (!XMLUtils::GetInt(pChannelNode, "encryption", channel.iEncryptionSystem))
        channel.iEncryptionSystem = 0;

      /* icon path */
      if (!XMLUtils::GetString(pChannelNode, "icon", strTmp))
        channel.strIconPath = m_strDefaultIcon;
      else
        channel.strIconPath = strTmp;

      /* stream url */
      if (!XMLUtils::GetString(pChannelNode, "stream", strTmp))
        channel.strStreamURL = m_strDefaultMovie;
      else
        channel.strStreamURL = strTmp;

      m_channels.push_back(channel);
    }
  }

  /* load channel groups */
  int iUniqueGroupId = 0;
  pElement = pRootElement->FirstChildElement("channelgroups");
  if (pElement)
  {
    TiXmlNode *pGroupNode = NULL;
    while ((pGroupNode = pElement->IterateChildren(pGroupNode)) != NULL)
    {
      CStdString strTmp;
      PVRDemoChannelGroup group;
      group.iGroupId = ++iUniqueGroupId;

      /* group name */
      if (!XMLUtils::GetString(pGroupNode, "name", strTmp))
        continue;
      group.strGroupName = strTmp;

      /* radio/TV */
      XMLUtils::GetBoolean(pGroupNode, "radio", group.bRadio);

      /* members */
      TiXmlNode* pMembers = pGroupNode->FirstChild("members");
      TiXmlNode *pMemberNode = NULL;
      while (pMembers != NULL && (pMemberNode = pMembers->IterateChildren(pMemberNode)) != NULL)
      {
        int iChannelId = atoi(pMemberNode->FirstChild()->Value());
        if (iChannelId > -1)
          group.members.push_back(iChannelId);
      }

      m_groups.push_back(group);
    }
  }

  /* load EPG entries */
  pElement = pRootElement->FirstChildElement("epg");
  if (pElement)
  {
    TiXmlNode *pEpgNode = NULL;
    while ((pEpgNode = pElement->IterateChildren(pEpgNode)) != NULL)
    {
      CStdString strTmp;
      int iTmp;
      PVRDemoEpgEntry entry;

      /* broadcast id */
      if (!XMLUtils::GetInt(pEpgNode, "broadcastid", entry.iBroadcastId))
        continue;

      /* channel id */
      if (!XMLUtils::GetInt(pEpgNode, "channelid", iTmp))
        continue;
      PVRDemoChannel &channel = m_channels.at(iTmp - 1);
      entry.iChannelId = channel.iUniqueId;

      /* title */
      if (!XMLUtils::GetString(pEpgNode, "title", strTmp))
        continue;
      entry.strTitle = strTmp;

      /* start */
      if (!XMLUtils::GetInt(pEpgNode, "start", iTmp))
        continue;
      entry.startTime = iTmp;

      /* end */
      if (!XMLUtils::GetInt(pEpgNode, "end", iTmp))
        continue;
      entry.endTime = iTmp;

      /* plot */
      if (XMLUtils::GetString(pEpgNode, "plot", strTmp))
        entry.strPlot = strTmp;

      /* plot outline */
      if (XMLUtils::GetString(pEpgNode, "plotoutline", strTmp))
        entry.strPlotOutline = strTmp;

      /* icon path */
      if (XMLUtils::GetString(pEpgNode, "icon", strTmp))
        entry.strIconPath = strTmp;

      /* genre type */
      XMLUtils::GetInt(pEpgNode, "genretype", entry.iGenreType);

      /* genre subtype */
      XMLUtils::GetInt(pEpgNode, "genresubtype", entry.iGenreSubType);

      XBMC->Log(LOG_DEBUG, "loaded EPG entry '%s' channel '%d' start '%d' end '%d'", entry.strTitle.c_str(), entry.iChannelId, entry.startTime, entry.endTime);
      channel.epg.push_back(entry);
    }
  }

  /* load recordings */
  iUniqueGroupId = 0; // reset unique ids
  pElement = pRootElement->FirstChildElement("recordings");
  if (pElement)
  {
    TiXmlNode *pRecordingNode = NULL;
    while ((pRecordingNode = pElement->IterateChildren(pRecordingNode)) != NULL)
    {
      CStdString strTmp;
      PVRDemoRecording recording;

      /* recording title */
      if (!XMLUtils::GetString(pRecordingNode, "title", strTmp))
        continue;
      recording.strTitle = strTmp;

      /* recording url */
      if (!XMLUtils::GetString(pRecordingNode, "url", strTmp))
        recording.strStreamURL = m_strDefaultMovie;
      else
        recording.strStreamURL = strTmp;

      /* recording path */
      if (XMLUtils::GetString(pRecordingNode, "directory", strTmp))
        recording.strDirectory = strTmp;

      iUniqueGroupId++;
      strTmp.Format("%d", iUniqueGroupId);
      recording.strRecordingId = strTmp;

      /* channel name */
      if (XMLUtils::GetString(pRecordingNode, "channelname", strTmp))
        recording.strChannelName = strTmp;

      /* plot */
      if (XMLUtils::GetString(pRecordingNode, "plot", strTmp))
        recording.strPlot = strTmp;

      /* plot outline */
      if (XMLUtils::GetString(pRecordingNode, "plotoutline", strTmp))
        recording.strPlotOutline = strTmp;

      /* genre type */
      XMLUtils::GetInt(pRecordingNode, "genretype", recording.iGenreType);

      /* genre subtype */
      XMLUtils::GetInt(pRecordingNode, "genresubtype", recording.iGenreSubType);

      /* duration */
      XMLUtils::GetInt(pRecordingNode, "duration", recording.iDuration);

      /* recording time */
      if (XMLUtils::GetString(pRecordingNode, "time", strTmp))
      {
        time_t timeNow = time(NULL);
        struct tm* now = localtime(&timeNow);

        CStdString::size_type delim = strTmp.Find(':');
        if (delim != CStdString::npos)
        {
          now->tm_hour = (int)strtol(strTmp.Left(delim), NULL, 0);
          now->tm_min  = (int)strtol(strTmp.Mid(delim + 1), NULL, 0);
          now->tm_mday--; // yesterday

          recording.recordingTime = mktime(now);
        }
      }

      m_recordings.push_back(recording);
    }
  }

  /* load timers */
  pElement = pRootElement->FirstChildElement("timers");
  if (pElement)
  {
    TiXmlNode *pTimerNode = NULL;
    while ((pTimerNode = pElement->IterateChildren(pTimerNode)) != NULL)
    {
      CStdString strTmp;
      int iTmp;
      PVRDemoTimer timer;
      time_t timeNow = time(NULL);
      struct tm* now = localtime(&timeNow);

      /* channel id */
      if (!XMLUtils::GetInt(pTimerNode, "channelid", iTmp))
        continue;
      PVRDemoChannel &channel = m_channels.at(iTmp - 1);
      timer.iChannelId = channel.iUniqueId;

      /* state */
      if (XMLUtils::GetInt(pTimerNode, "state", iTmp))
        timer.state = (PVR_TIMER_STATE) iTmp;

      /* title */
      if (!XMLUtils::GetString(pTimerNode, "title", strTmp))
        continue;
      timer.strTitle = strTmp;

      /* summary */
      if (!XMLUtils::GetString(pTimerNode, "summary", strTmp))
        continue;
      timer.strSummary = strTmp;

      /* start time */
      if (XMLUtils::GetString(pTimerNode, "starttime", strTmp))
      {
        CStdString::size_type delim = strTmp.Find(':');
        if (delim != CStdString::npos)
        {
          now->tm_hour = (int)strtol(strTmp.Left(delim), NULL, 0);
          now->tm_min  = (int)strtol(strTmp.Mid(delim + 1), NULL, 0);

          timer.startTime = mktime(now);
        }
      }

      /* end time */
      if (XMLUtils::GetString(pTimerNode, "endtime", strTmp))
      {
        CStdString::size_type delim = strTmp.Find(':');
        if (delim != CStdString::npos)
        {
          now->tm_hour = (int)strtol(strTmp.Left(delim), NULL, 0);
          now->tm_min  = (int)strtol(strTmp.Mid(delim + 1), NULL, 0);

          timer.endTime = mktime(now);
        }
      }

      XBMC->Log(LOG_DEBUG, "loaded timer '%s' channel '%d' start '%d' end '%d'", timer.strTitle.c_str(), timer.iChannelId, timer.startTime, timer.endTime);
      m_timers.push_back(timer);
    }
  }

  return true;
}

int PVRDemoData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR PVRDemoData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRDemoChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      xbmcChannel.iSubChannelNumber = channel.iSubChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
      strncpy(xbmcChannel.strStreamURL, channel.strStreamURL.c_str(), sizeof(xbmcChannel.strStreamURL) - 1);
      xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

const char* PVRDemoData::GetChannelStreamURL(const PVR_CHANNEL &channel)
{
	for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
	{
		PVRDemoChannel &thisChannel = m_channels.at(iChannelPtr);

		if (thisChannel.iUniqueId == (int)channel.iUniqueId)
		{
			XBMC->Log(LOG_ERROR, "channel found. getting temp stream url\n");

			string url;
			struct MemoryStruct chunk;
			Json::Value parsedFromString;
			Json::Reader reader;
			bool parsingSuccessful;

			url = "?type=itv&action=create_link&cmd=ffrt2%20" + thisChannel.strStreamURL2 + "&forced_storage=undefined&disable_ad=0&JsHttpRequest=1-xml&";

			chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			chunk.size = 0;    /* no data at this point */

			if (!DoAPICall(&url, &chunk)) {
				XBMC->Log(LOG_ERROR, "API call failed\n");

				free(chunk.memory);
				break;
			}

			parsingSuccessful = reader.parse(chunk.memory, parsedFromString);

			free(chunk.memory);

			if (!parsingSuccessful) {
				break;
			}

			string cmd = parsedFromString["js"]["cmd"].asString();
			regex cmdRegex("ffrt2\\s(.+)");
			smatch cmdMatch;
			if (!regex_match(cmd, cmdMatch, cmdRegex)) {
				XBMC->Log(LOG_ERROR, "no stream url. skipping\n");
				break;
			}

			m_PlaybackURL = cmdMatch[1].str();
			
			return m_PlaybackURL.c_str();
		}
	}

	return "";
}

bool PVRDemoData::GetChannel(const PVR_CHANNEL &channel, PVRDemoChannel &myChannel)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
	PVRDemoChannel &thisChannel = m_channels.at(iChannelPtr);
	if (thisChannel.iUniqueId == (int) channel.iUniqueId)
	{
		myChannel.iUniqueId         = thisChannel.iUniqueId;
		myChannel.bRadio            = thisChannel.bRadio;
		myChannel.iChannelNumber    = thisChannel.iChannelNumber;
		myChannel.iSubChannelNumber = thisChannel.iSubChannelNumber;
		myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
		myChannel.strChannelName    = thisChannel.strChannelName;
		myChannel.strIconPath       = thisChannel.strIconPath;
		myChannel.strStreamURL      = thisChannel.strStreamURL;

		return true;
	}
  }

  return false;
}

int PVRDemoData::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR PVRDemoData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRDemoChannelGroup &group = m_groups.at(iGroupPtr);
    if (group.bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup;
      memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

      xbmcGroup.bIsRadio = bRadio;
      strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRDemoData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRDemoChannelGroup &myGroup = m_groups.at(iGroupPtr);
    if (!strcmp(myGroup.strGroupName.c_str(),group.strGroupName))
    {
      for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
      {
        int iId = myGroup.members.at(iChannelPtr) - 1;
        if (iId < 0 || iId > (int)m_channels.size() - 1)
          continue;
        PVRDemoChannel &channel = m_channels.at(iId);
        PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
        memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

        strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
        xbmcGroupMember.iChannelUniqueId  = channel.iUniqueId;
        xbmcGroupMember.iChannelNumber    = channel.iChannelNumber;

        PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRDemoData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (m_iEpgStart == -1)
    m_iEpgStart = iStart;

  time_t iLastEndTime = m_iEpgStart + 1;
  int iAddBroadcastId = 0;

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRDemoChannel &myChannel = m_channels.at(iChannelPtr);
    if (myChannel.iUniqueId != (int) channel.iUniqueId)
      continue;

    while (iLastEndTime < iEnd && myChannel.epg.size() > 0)
    {
      time_t iLastEndTimeTmp = 0;
      for (unsigned int iEntryPtr = 0; iEntryPtr < myChannel.epg.size(); iEntryPtr++)
      {
        PVRDemoEpgEntry &myTag = myChannel.epg.at(iEntryPtr);

        EPG_TAG tag;
        memset(&tag, 0, sizeof(EPG_TAG));

        tag.iUniqueBroadcastId = myTag.iBroadcastId + iAddBroadcastId;
        tag.strTitle           = myTag.strTitle.c_str();
        tag.iChannelNumber     = myTag.iChannelId;
        tag.startTime          = myTag.startTime + iLastEndTime;
        tag.endTime            = myTag.endTime + iLastEndTime;
        tag.strPlotOutline     = myTag.strPlotOutline.c_str();
        tag.strPlot            = myTag.strPlot.c_str();
        tag.strIconPath        = myTag.strIconPath.c_str();
        tag.iGenreType         = myTag.iGenreType;
        tag.iGenreSubType      = myTag.iGenreSubType;

        iLastEndTimeTmp = tag.endTime;

        PVR->TransferEpgEntry(handle, &tag);
      }

      iLastEndTime = iLastEndTimeTmp;
      iAddBroadcastId += myChannel.epg.size();
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRDemoData::GetRecordingsAmount(void)
{
  return m_recordings.size();
}

PVR_ERROR PVRDemoData::GetRecordings(ADDON_HANDLE handle)
{
  for (std::vector<PVRDemoRecording>::iterator it = m_recordings.begin() ; it != m_recordings.end() ; it++)
  {
    PVRDemoRecording &recording = *it;

    PVR_RECORDING xbmcRecording;

    xbmcRecording.iDuration     = recording.iDuration;
    xbmcRecording.iGenreType    = recording.iGenreType;
    xbmcRecording.iGenreSubType = recording.iGenreSubType;
    xbmcRecording.recordingTime = recording.recordingTime;

    strncpy(xbmcRecording.strChannelName, recording.strChannelName.c_str(), sizeof(xbmcRecording.strChannelName) - 1);
    strncpy(xbmcRecording.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(xbmcRecording.strPlotOutline) - 1);
    strncpy(xbmcRecording.strPlot,        recording.strPlot.c_str(),        sizeof(xbmcRecording.strPlot) - 1);
    strncpy(xbmcRecording.strRecordingId, recording.strRecordingId.c_str(), sizeof(xbmcRecording.strRecordingId) - 1);
    strncpy(xbmcRecording.strTitle,       recording.strTitle.c_str(),       sizeof(xbmcRecording.strTitle) - 1);
    strncpy(xbmcRecording.strStreamURL,   recording.strStreamURL.c_str(),   sizeof(xbmcRecording.strStreamURL) - 1);
    strncpy(xbmcRecording.strDirectory,   recording.strDirectory.c_str(),   sizeof(xbmcRecording.strDirectory) - 1);

    PVR->TransferRecordingEntry(handle, &xbmcRecording);
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRDemoData::GetTimersAmount(void)
{
  return m_timers.size();
}

PVR_ERROR PVRDemoData::GetTimers(ADDON_HANDLE handle)
{
  int i = 0;
  for (std::vector<PVRDemoTimer>::iterator it = m_timers.begin() ; it != m_timers.end() ; it++)
  {
    PVRDemoTimer &timer = *it;

    PVR_TIMER xbmcTimer;
    memset(&xbmcTimer, 0, sizeof(PVR_TIMER));

    xbmcTimer.iClientIndex      = ++i;
    xbmcTimer.iClientChannelUid = timer.iChannelId;
    xbmcTimer.startTime         = timer.startTime;
    xbmcTimer.endTime           = timer.endTime;
    xbmcTimer.state             = timer.state;

    strncpy(xbmcTimer.strTitle, timer.strTitle.c_str(), sizeof(timer.strTitle) - 1);
    strncpy(xbmcTimer.strSummary, timer.strSummary.c_str(), sizeof(timer.strSummary) - 1);

    PVR->TransferTimerEntry(handle, &xbmcTimer);
  }

  return PVR_ERROR_NO_ERROR;
}
