#pragma once

#include <string>
#include <vector>

class HTTPSocket
{
public:
	HTTPSocket();
	virtual ~HTTPSocket();

	void SetURL(const std::string &url);
	void SetBody(const std::string &req_body);
	void AddHeader(const std::string &name, const std::string &value);
	bool Execute(std::string *resp_headers, std::string *resp_body);
protected:
	virtual bool BuildRequest(std::string *request);
	virtual bool OpenSocket(int *sockfd);
	virtual bool CloseSocket(int *sockfd);
private:
	std::string m_method;
	std::string m_uri;
	std::string m_host;
	int m_port;
	std::string m_user_agent;
	std::vector<std::string> m_headers;
	std::string m_req_body;
};

