#pragma once
#include <iostream>
#include <string>
#include <sstream>

class CHttpSocket
{
public:
	virtual ~CHttpSocket();

	static CHttpSocket* Instance();
	std::string socketHttp(const char* host, const char* request);

private:
	void Convert(const char* strIn, std::string& strOut, int sourceCodepage, int targetCodepage);
private:
	CHttpSocket();
	static CHttpSocket* s_instance;
	std::stringstream stream;
	std::string stringbuff;
	int sockfd;
	char* buf;
};