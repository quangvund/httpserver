#include "httpserver.h"
#include <thread>
#include <map>

HTTPServer::HTTPServer(int port)
{
	m_nPort = port;
	m_bServerRunning = false;
	m_listenSocket = INVALID_SOCKET;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		Error("WSAStarup failed");
}
HTTPServer::~HTTPServer()
{
	WSACleanup();
}
int HTTPServer::start()
{
	struct  addrinfo* result = NULL;
	struct addrinfo hints;;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, std::to_string(m_nPort).c_str(), &hints, &result) != 0)
	{
		return Error("getaddrinfo failed with error");
	}
	m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (m_listenSocket == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		return Error("socket failed with error");
	}
	if (bind(m_listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		freeaddrinfo(result);
		return Error("bind failed with error");
	}
	freeaddrinfo(result);
	if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(m_listenSocket);
		return Error("listen failed with error");
	}
	std::map<SOCKET, std::thread> threads;

	m_bServerRunning = true;
	while (m_bServerRunning)
	{
		SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			closesocket(m_listenSocket);
			return Error("accept failed with error");
		}

		threads[clientSocket] = std::thread(&HTTPServer::clientHandler, this, clientSocket);
		threads[clientSocket].detach(); 
	}

	return 1;
}
void HTTPServer::stop()
{
	m_bServerRunning = false;
	if (m_listenSocket != INVALID_SOCKET)
		closesocket(m_listenSocket);
}
int HTTPServer::Error(const char* msg)
{
	wchar_t buff[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buff,
		256,
		NULL);
	stop();
	std::cout << msg << std::endl << buff << std::endl;

	return 0;
}