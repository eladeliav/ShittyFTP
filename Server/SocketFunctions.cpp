#include "pch.h"
#include "SocketFunctions.h"

bool initServer(WSAData & wsaData, addrinfo & hints, addrinfo *& result, SOCKET & listenSock)
{
	if (!initWinsock(wsaData))
		return false;

	if (!configureSocketAdressInfo(hints, result))
		return false;

	if (!configureSocketHint(listenSock, result))
		return false;

	if (!bindSocket(listenSock, result))
		return false;

	freeaddrinfo(result);

	if (!listenFromSock(listenSock))
		return false;
	return true;
}

bool initWinsock(WSAData & wsaData)
{
	//initializing winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "Failed to init winsock with error: " << iResult << std::endl;
		return false;
	}
	return true;
}

bool configureSocketAdressInfo(addrinfo & hints, addrinfo *& result)
{
	//zeroing out the hints struct memory
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; //ipv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int iResult = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		std::cout << "Get addr info failed with error: " << iResult << std::endl;
		WSACleanup();
		return false;
	}
	return true;
}

bool configureSocketHint(SOCKET & sock, addrinfo *& result)
{
	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Failed to initialize listening socket error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}
	return true;
}

bool bindSocket(SOCKET & sock, addrinfo *& result)
{
	int iResult = bind(sock, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Failed to bind listening socket error: " << iResult << std::endl;
		freeaddrinfo(result);
		closesocket(sock);
		WSACleanup();
		return false;
	}
	return true;
}

bool listenFromSock(SOCKET & sock)
{
	int iResult = listen(sock, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Failed to listen from listening socket error: " << iResult << std::endl;
		closesocket(sock);
		WSACleanup();
		return false;
	}
	return true;
}

SOCKET acceptSocket(SOCKET & listenSock)
{
	SOCKET clientSock = accept(listenSock, NULL, NULL);
	if (clientSock == INVALID_SOCKET)
	{
		std::cout << "Failed to accept client socket error: " << WSAGetLastError() << std::endl;
		closesocket(listenSock);
		WSACleanup();
		return INVALID_SOCKET;
	}
	return clientSock;
}

int sockClose(SOCKET & sock)
{
	int status = 0;
	status = shutdown(sock, SD_BOTH);
	if (status == 0) { status = closesocket(sock); }

	return status;
}

void sendMessageToSocket(string message, SOCKET &sock)
{
	int sendStatus = send(sock, message.c_str(), (int)message.size(), 0);
}

bool newClient(SOCKET &listenSock, fd_set &master)
{
	SOCKET newClient = acceptSocket(listenSock);
	if (newClient == INVALID_SOCKET)
		return false;
	FD_SET(newClient, &master);
	sendMessageToSocket(WELCOME_MESSAGE, newClient);
	std::cout << "New client " << newClient << " connected" << std::endl;
	return true;
}