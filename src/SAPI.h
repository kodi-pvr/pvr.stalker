#pragma once

#include <string>
#include <jsoncpp/include/json/json.h>

#include "HTTPSocket.h"

#define AUTHORIZATION_FAILED "Authorization failed."

namespace SAPI
{
	bool Init();
	bool StalkerCall(HTTPSocket *sock, std::string *resp_headers, std::string *resp_body, Json::Value *parsed);
	bool Handshake(Json::Value *parsed);
	bool GetProfile(Json::Value *parsed);
	bool GetAllChannels(Json::Value *parsed);
	bool CreateLink(std::string &cmd, Json::Value *parsed);
};

