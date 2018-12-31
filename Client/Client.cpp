// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#define DEFAULT_PORT "5400"
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_BUFFER_LEN 512
#pragma warning(disable : 4996)

using std::thread;
bool connected = false;

bool initWinsock(WSAData & wsaData);
bool configureSocketAdressInfo(addrinfo & hints, addrinfo *& result);
bool configureSocketHint(SOCKET & sock, addrinfo *& result);
bool connectToServer(SOCKET &sock, addrinfo *&result);
bool initClient(WSAData &wsaData, SOCKET &connectSocket, addrinfo &hints, addrinfo *&result);
void sendMessageToSocket(string message, SOCKET &sock);
int sockClose(SOCKET & sock);
void sendMessages(SOCKET &sock, std::string &userInput);


int main()
{
	WSAData wsaData;
	SOCKET connectSocket = INVALID_SOCKET;
	char Buf[DEFAULT_BUFFER_LEN];
	struct addrinfo hints;
	struct addrinfo *result;

	if (!initClient(wsaData, connectSocket, hints, result))
		return 1;

	string userInput = " ";
	thread sendMessagesThread(sendMessages, std::ref(connectSocket), std::ref(userInput));
	sendMessagesThread.detach();
	while(connected)
	{
		ZeroMemory(Buf, DEFAULT_BUFFER_LEN);
		int bytesReceived = recv(connectSocket, Buf, DEFAULT_BUFFER_LEN, 0);
		if (bytesReceived > 0)
		{
			string outStr = string(Buf, 0, bytesReceived);
			std::cout << outStr << std::endl;
			std::cout << "> ";
			if (outStr.find("SENDING_FILE") != string::npos)
			{
				string basename = outStr;
				if (outStr.find('\\') != string::npos)
					basename = basename.substr(outStr.rfind('\\') + 1);
				else
					basename = outStr.substr(outStr.find(' ') + 1);

				string fileName = "received_" + basename;
				//std::cout << "extracted file name " << fileName << std::endl;
				FILE *filehandle = fopen(fileName.c_str(), "wb");
				if (filehandle != NULL)
				{
					bool ok = readfile(connectSocket, filehandle);
					fclose(filehandle);
				}
			}
		}
		else
			connected = false;
	}

	sockClose(connectSocket);
	WSACleanup();
	return 0;
}

bool readdata(SOCKET &sock, void* buf, int buflen)
{
	char* pBuf = (char*)buf;
	while (buflen > 0)
	{
		int num = recv(sock, pBuf, buflen, 0);
		if (num == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// optional: use select() to check for timeout to fail the read
				continue;
			}
			return false;
		}
		else if (num == 0)
			return false;

		pBuf += num;
		buflen -= num;
	}

	return true;
}

bool readlong(SOCKET& sock, long& value)
{
	if (!readdata(sock, &value, sizeof(value)))
		return false;
	return true;
}

bool readfile(SOCKET& sock, FILE* f)
{
	long filesize = 0;
	if (!readlong(sock, filesize))
		return false;
	if (filesize > 0)
	{
		char buffer[1024];
		do
		{
			int num = min(filesize, sizeof(buffer));
			if (!readdata(sock, buffer, num))
				return false;
			int offset = 0;
			do
			{
				size_t written = fwrite(&buffer[offset], 1, num - offset, f);
				if (written < 1)
					return false;
				offset += (int)written;
			} while (offset < num);
			filesize -= num;
		} while (filesize > 0);
	}
	return true;
}

void sendMessages(SOCKET &sock, std::string &userInput)
{
	while(connected)
	{
		std::cout << "> ";
		std::cin >> std::ws;
		std::getline(std::cin, userInput, '\n');

		boost::algorithm::to_upper(userInput);
		std::string shutdownServerMessage = "SERVER_SHUTDOWN";
		std::string shutdownClientMessage = "EXIT";

		if (userInput == shutdownServerMessage)
		{
			//sending the command
			send(sock, userInput.c_str(), (int)userInput.size(), 0);

			connected = false;
			std::cout << "Exiting..." << std::endl;
			sockClose(sock);
			WSACleanup();
			return;
		}
		else if (userInput == shutdownClientMessage)
		{
			connected = false;
			std::cout << "Exiting..." << std::endl;
			sockClose(sock);
			WSACleanup();
			return;
		}

		if (userInput.size() > 0)
		{
			sendMessageToSocket(userInput, sock);
		}
	}
}

bool initClient(WSAData &wsaData, SOCKET &connectSocket, addrinfo &hints, addrinfo *&result)
{
	//initializing winsock
	if (!initWinsock(wsaData))
		return false;

	if (!configureSocketAdressInfo(hints, result))
		return false;

	if (!configureSocketHint(connectSocket, result))
		return false;

	if (!connectToServer(connectSocket, result))
		return false;

	freeaddrinfo(result);
	connected = true;
	return true;
}

bool connectToServer(SOCKET &sock, addrinfo *&result)
{
	int iResult = connect(sock, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		sockClose(sock);
		std::cout << "Failed to connect to server: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}
	return true;
}

bool initWinsock(WSAData &wsaData)
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

void sendMessageToSocket(string message, SOCKET &sock)
{
	string command;
	vector<string> params;
	splitRequestAndParams(message, command, params);
//	std::cout << "deteceted command " << command << std::endl;
	int sendStatus = send(sock, message.c_str(), (int)message.size(), 0);
}

void splitRequestAndParams(string commandAndParams, string &command, vector<string> &paramsVector)
{
	command = commandAndParams;
	if (commandAndParams.find(' ') != string::npos)
	{
		command = command.erase(commandAndParams.find(' '));

		string params = commandAndParams.substr(commandAndParams.find(' ') + 1);

		std::stringstream test(params);
		std::string segment;
		while (std::getline(test, segment, ' '))
		{
			paramsVector.push_back(segment);
		}
	}
}

int sockClose(SOCKET & sock)
{
	int status = 0;
	status = shutdown(sock, SD_BOTH);
	if (status == 0) { status = closesocket(sock); }

	return status;
}
