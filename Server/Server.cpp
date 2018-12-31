// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#pragma warning(disable : 4996) //disable "unsafe function" warning


bool running = true; //server is funning flag
typedef std::string(*simpleCommandPtr)(vector<string>); //typedef for function pointer string x(vector<string>)
typedef std::string(*socketCommandPtr)(vector<string>,SOCKET&); //typedef for function pointer x(vector<string>, SOCKET)
const std::map<string, simpleCommandPtr> COMMAND_MAP = 
{
	{"PING", &pong},
	{"SERVER_SHUTDOWN", &shutdown_server},
	{"ADD", &additionCommand},
	{"TAKE_SCREENSHOT", &take_screenshot},
	{"DIR", &directoryCommand},
	{"DELETE", &deleteFileCommand},
};

const std::map<string, socketCommandPtr> SOCKET_COMMAND_MAP =
{
	{"SEND_FILE", &sendFileCommand}
};

const std::map<string, int> NUM_OF_PARAMS = 
{
	{"PING", 0},
	{"SERVER_SHUTDOWN", 0},
	{"ADD", 2},
	{"TAKE_SCREENSHOT", 0},
	{"SEND_FILE", 1},
	{"DIR", 1},
	{"DELETE", 1},
};

int main()
{
	WSAData wsaData;

	struct addrinfo hints;
	struct addrinfo *result;

	SOCKET listenSock = INVALID_SOCKET;

	char Buf[DEFAULT_BUFFER_LEN];

	//initializing everything for the server
	if (!initServer(wsaData, hints, result, listenSock))
		return 1;

	//declaring fd_set (socket set)
	fd_set master;
	FD_ZERO(&master);

	FD_SET(listenSock, &master); //adding the listening socket to the set
	while (running)
	{
		fd_set copy = master; //copying the master fd_set so we don't fuck it
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++)
		{
			SOCKET currentSocket = copy.fd_array[i];
			if (currentSocket == listenSock)
			{
				//potential new client
				if (!newClient(listenSock, master))
				{
					continue;
				}
			}
			else
			{
				handleClientRequest(currentSocket, master, Buf);
			}
		}
		//foreach socket:
		//	accept if new
		//	accept and process inboud commands

	}
	
	sockClose(listenSock);
	WSACleanup();
	return 0;
}

bool senddata(SOCKET sock, void *buf, int buflen)
{
	char *pbuf = (char *)buf;

	while (buflen > 0)
	{
		int num = send(sock, pbuf, buflen, 0);
		if (num == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// optional: use select() to check for timeout to fail the send
				continue;
			}
			return false;
		}

		pbuf += num;
		buflen -= num;
	}

	return true;
}

bool sendlong(SOCKET sock, long& value)
{
	return senddata(sock, &value, sizeof(value));
}

bool sendfile(SOCKET sock, FILE *f, string& fileName)
{
	fseek(f, 0, SEEK_END);
	long filesize = ftell(f);
	rewind(f);
	if (filesize == EOF)
		return false;
	if (!sendlong(sock, filesize))
		return false;
	if (filesize > 0)
	{
		char buffer[1024];
		do
		{
			size_t num = min(filesize, sizeof(buffer));
			num = fread(buffer, 1, num, f);
			if (num < 1)
				return false;
			if (!senddata(sock, buffer, (int)num))
				return false;
			filesize -= (long)num;
		} while (filesize > 0);
	}
	return true;
}

string sendFileCommand(vector<string> params,SOCKET &sock)
{
	std::cout << "about to open " << params[0] << std::endl;
	FILE *filehandle = fopen(params[0].c_str(), "rb");
	if (filehandle != NULL)
	{
		string toSend = "SENDING_FILE " + params[0];
		sendMessageToSocket(toSend, sock);
		sendfile(sock, filehandle, params[0]);
		fclose(filehandle);
		return "File Sent successfully";
	}
	return "File failed to send";
}

void handleClientRequest(SOCKET &sock, fd_set &master, char *Buf)
{
	//inbound message
	ZeroMemory(Buf, DEFAULT_BUFFER_LEN);
	int receivedBytes = recv(sock, Buf, DEFAULT_BUFFER_LEN, 0);
	if (receivedBytes <= 0)
	{
		std::cout << "Client " << sock << " Disconnected" << std::endl;
		sockClose(sock);
		FD_CLR(sock, &master);
	}
	else
	{
		string commandAndParams = string(Buf, receivedBytes);
		std::cout << "Detected request: " << commandAndParams << ", size: " << commandAndParams.size() << std::endl;

		string command = commandAndParams;
		vector<string> paramsVector;
		splitRequestAndParams(commandAndParams, command, paramsVector);
		boost::algorithm::to_upper(command);
		std::cout << "Extracted Command: " << command << std::endl;
		auto mapPair = COMMAND_MAP.find(command);
		auto numOfParamsPair = NUM_OF_PARAMS.find(command);
		auto socketMapPair = SOCKET_COMMAND_MAP.find(command);

		if (mapPair != COMMAND_MAP.end() && numOfParamsPair != NUM_OF_PARAMS.end()) //command in map
		{
			auto commandFuncPtr = mapPair->second;
			int numOfParams = numOfParamsPair->second;
			if (paramsVector.size() != numOfParams)
			{
				std::ostringstream ss;
				ss << "Wrong use of parameters. Use: " << command << " " << numOfParams << "# of params" << std::endl;
				string strOut = ss.str();
				sendMessageToSocket(strOut, sock);
				return;
			}
			string response = commandFuncPtr(paramsVector);

			if (response == "Server Shutting Down")
			{
				running = false;
			}
			sendMessageToSocket(response, sock);
		}
		else if (socketMapPair != SOCKET_COMMAND_MAP.end() && numOfParamsPair != NUM_OF_PARAMS.end())
		{
			auto commandFuncPtr = socketMapPair->second;
			int numOfParams = numOfParamsPair->second;
			if (paramsVector.size() != numOfParams)
			{
				std::ostringstream ss;
				ss << "Wrong use of parameters. Use: " << command << " " << numOfParams << "# of params" << std::endl;
				string strOut = ss.str();
				sendMessageToSocket(strOut, sock);
				return;
			}
			string response = commandFuncPtr(paramsVector, sock);
			sendMessageToSocket(response, sock);
		}
		else
			sendMessageToSocket("Command Unknown", sock);
	}
}
