#include "SAPI.h"

#include "client.h"

using namespace ADDON;

namespace SAPI
{
	bool Init()
	{
		HTTPSocket sock;
		std::string resp_headers;
		std::string resp_body;
		size_t pos;
		std::string locationUrl;

		sock.SetURL(g_strServer);

		if (!sock.Execute(&resp_headers, &resp_body)) {
			XBMC->Log(LOG_ERROR, "%s: api init failed\n", __FUNCTION__);
			return false;
		}

		// xpcom.common.js > get_server_params()

		if ((pos = resp_headers.find("Location: ")) == std::string::npos) {
			XBMC->Log(LOG_ERROR, "%s: failed to get api endpoint\n", __FUNCTION__);
			return false;
		}

		locationUrl = resp_headers.substr(pos + 10, resp_headers.find("\r\n", pos) - (pos + 10));

		pos = locationUrl.find_last_of("/");
		g_referrer = locationUrl.substr(0, pos + 1);

		if (g_referrer.substr(pos - 2).compare("/c/") == 0) {
			g_api_endpoint = g_referrer.substr(0, pos - 1) + "server/load.php";
		}

		XBMC->Log(LOG_ERROR, "api endpoint: %s\n", g_api_endpoint.c_str());
		XBMC->Log(LOG_ERROR, "referrer: %s\n", g_referrer.c_str());

		return true;
	}

	bool StalkerCall(HTTPSocket *sock, std::string *resp_headers, std::string *resp_body, Json::Value *parsed)
	{
		size_t pos;
		Json::Reader reader;

		std::string cookie = "mac=" + g_strMac + "; stb_lang=en; timezone=Europe%2FKiev";
		while ((pos = cookie.find(":")) != std::string::npos) {
			cookie.replace(pos, 1, "%3A");
		}
		sock->AddHeader("Cookie", cookie);

		sock->AddHeader("Referer", g_referrer);
		sock->AddHeader("X-User-Agent", "Model: MAG250; Link: WiFi");
		sock->AddHeader("Authorization", "Bearer " + g_token);

		if (!sock->Execute(resp_headers, resp_body)) {
			XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
			return false;
		}

		if (!reader.parse(*resp_body, *parsed)) {
			XBMC->Log(LOG_ERROR, "%s: parsing failed\n", __FUNCTION__);
			if (resp_body->compare(AUTHORIZATION_FAILED) == 0) {
				XBMC->Log(LOG_ERROR, "%s: authorization failed\n", __FUNCTION__);
				g_authorized = false;
			}
			return false;
		}

		return true;
	}

	bool Handshake(Json::Value *parsed)
	{
		HTTPSocket sock;
		std::string resp_headers;
		std::string resp_body;

		sock.SetURL(g_api_endpoint + "?type=stb&action=handshake&JsHttpRequest=1-xml&");

		if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
			XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
			return false;
		}

		g_token = (*parsed)["js"]["token"].asString();

		XBMC->Log(LOG_ERROR, "token: %s\n", g_token.c_str());

		return true;
	}

	bool GetProfile(Json::Value *parsed)
	{
		HTTPSocket sock;
		std::string resp_headers;
		std::string resp_body;

		sock.SetURL(g_api_endpoint + "?type=stb&action=get_profile"
			"&hd=1&ver=ImageDescription:%200.2.16-250;%20ImageDate:%2018%20Mar%202013%2019:56:53%20GMT+0200;%20PORTAL%20version:%204.9.9;%20API%20Version:%20JS%20API%20version:%20328;%20STB%20API%20version:%20134;%20Player%20Engine%20version:%200x560"
			"&num_banks=1&sn=0000000000000&stb_type=MAG250&image_version=216&auth_second_step=0&hw_version=1.7-BD-00&not_valid_token=0&JsHttpRequest=1-xml&");

		if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
			XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
			return false;
		}

		return true;
	}

	bool GetAllChannels(Json::Value *parsed)
	{
		HTTPSocket sock;
		std::string resp_headers;
		std::string resp_body;

		sock.SetURL(g_api_endpoint + "?type=itv&action=get_all_channels&JsHttpRequest=1-xml&");

		if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
			XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
			return false;
		}

		return true;
	}

	bool CreateLink(std::string &cmd, Json::Value *parsed)
	{
		HTTPSocket sock;
		std::string resp_headers;
		std::string resp_body;

		std::string tmp = cmd;
		tmp.replace(tmp.find(" "), 1, "%20");

		sock.SetURL(g_api_endpoint + "?type=itv&action=create_link&cmd=" + tmp + "&forced_storage=undefined&disable_ad=0&JsHttpRequest=1-xml&");

		if (!StalkerCall(&sock, &resp_headers, &resp_body, parsed)) {
			XBMC->Log(LOG_ERROR, "%s: api call failed\n", __FUNCTION__);
			return false;
		}

		return true;
	}
}
