//SERVER PCH
#ifndef PCH_H
#define PCH_H

//Basic includes
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <map>
#include <Windows.h>
#include <gdiplus.h>

//using vector and string

//using winsock and gdiplus libs
#pragma comment(lib, "gdiplus.lib")
#pragma comment (lib, "Ws2_32.lib")

//Constant primitive defines
#define DEFAULT_PORT "5400"
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_BUFFER_LEN 512
#define WELCOME_MESSAGE "Welcome to my shitty ftp shell\n"

//making L"string" work
#ifdef  UNICODE                     
#define _TEXT(quote) L##quote  
#else   /* UNICODE */               
#define _TEXT(quote) quote 
#endif /* UNICODE */    

//using string and vector
using std::vector;
using std::string;

//Function headers include
#include "SocketFunctions.h"
#include "CommandFunctions.h"



//send_file functions
bool senddata(SOCKET sock, void *buf, int buflen);
bool sendlong(SOCKET sock, long& value);
bool sendfile(SOCKET sock, FILE *f, string& fileName);
string sendFileCommand(vector<string> params,SOCKET &sock);
bool sendString(SOCKET sock, string& value);

void handleClientRequest(SOCKET &sock, fd_set &master, char *Buf);

bool isNumber(const char* pStr);

#endif //PCH_H
