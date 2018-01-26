#include "HttpSocket.h"

#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

CHttpSocket::CHttpSocket()
{
	WSADATA wsa = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsa);
	buf = new char[1024*1024];

}

CHttpSocket::~CHttpSocket()
{
	if (buf)
	{
		delete[] buf;
	}
}

CHttpSocket* CHttpSocket::Instance()
{
	if (s_instance == NULL)
	{
		//Î´¼ÓËø
		s_instance = new CHttpSocket();
	}

	return s_instance;
}

void CHttpSocket::Convert(const char* strIn, std::string& strOut, int sourceCodepage, int targetCodepage)
{
	strOut.clear();
	int len = lstrlen(strIn);
	int unicodeLen = MultiByteToWideChar(sourceCodepage, 0, strIn, -1, NULL, 0);

	wchar_t* pUnicode = NULL;
	pUnicode = new wchar_t[unicodeLen + 1];
	memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
	MultiByteToWideChar(sourceCodepage, 0, strIn, -1, (LPWSTR)pUnicode, unicodeLen);

	BYTE * pTargetData = NULL;
	int targetLen = WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode, -1, (char *)pTargetData, 0, NULL, NULL);

	pTargetData = new BYTE[targetLen + 1];
	memset(pTargetData, 0, targetLen + 1);
	WideCharToMultiByte(targetCodepage, 0, (LPWSTR)pUnicode, -1, (char *)pTargetData, targetLen, NULL, NULL);
	strOut = (char*)pTargetData;

	delete[] pUnicode;
	delete[] pTargetData;
}

std::string CHttpSocket::socketHttp(const char* host, const char* request)
{
	int sockfd;
	struct sockaddr_in address;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	address.sin_family = AF_INET;
	address.sin_port = htons(8000);
	server = gethostbyname(host);
	if (!server)
		return "";

	memcpy((char *)&address.sin_addr.s_addr, (char*)server->h_addr, server->h_length);

	if (-1 == connect(sockfd, (struct sockaddr *)&address, sizeof(address))){
		return "connection error!";
	}

	send(sockfd, request, strlen(request), 0);
	memset(buf, 0x00, 1024 * 1024);
	int offset = 0;
	int rc;

	while (rc = recv(sockfd, buf + offset, 1024, 0))
	{
		offset += rc;
	}

	closesocket(sockfd);
	buf[offset] = 0;
	std::string strret("");
	Convert(buf,strret,CP_UTF8,CP_ACP);

	return strret;
}