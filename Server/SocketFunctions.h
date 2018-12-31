#pragma once
//init fucntions
bool initServer(WSAData &wsaData, addrinfo &hints, addrinfo *&result, SOCKET &listenSock);
bool initWinsock(WSAData &wsaData);
bool configureSocketAdressInfo(addrinfo &hints, addrinfo *&result);
bool configureSocketHint(SOCKET &sock, addrinfo *&result);
bool bindSocket(SOCKET &sock, addrinfo *&result);
bool listenFromSock(SOCKET &sock);

//misc socket functions
SOCKET acceptSocket(SOCKET &listenSock);
int sockClose(SOCKET &sock);
void sendMessageToSocket(string message, SOCKET &sock);
bool newClient(SOCKET &listenSock, fd_set &master);