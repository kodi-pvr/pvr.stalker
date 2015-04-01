#pragma once

#include <string>
#include <stdint.h>
#include <jsoncpp/include/json/json.h>

#include "HTTPSocket.h"

#define AUTHORIZATION_FAILED "Authorization failed."

namespace SAPI
{
	bool Init();
  bool StalkerCall(HTTPSocket *sock, std::string *resp_headers, std::string *resp_body, Json::Value *parsed, std::string *strFilePath = NULL);
	bool Handshake(Json::Value *parsed);
	bool GetProfile(Json::Value *parsed);
	bool GetAllChannels(Json::Value *parsed);
	bool GetOrderedList(uint32_t page, Json::Value *parsed);
	bool CreateLink(std::string &cmd, Json::Value *parsed);
  bool GetWeek(Json::Value *parsed);
  bool GetSimpleDataTable(uint32_t channelId, std::string &date, uint32_t page, Json::Value *parsed);
  bool GetDataTable(uint32_t channelId, std::string &from, std::string &to, uint32_t page, Json::Value *parsed);
};
