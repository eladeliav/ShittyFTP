//CLIENT PCH
#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#pragma comment (lib, "Ws2_32.lib")
using std::string;
using std::vector;
//send file functions
bool readdata(SOCKET &sock, void* buf, int buflen);
bool readlong(SOCKET &sock, long& value);
bool readfile(SOCKET &sock, FILE *f);
void splitRequestAndParams(string commandAndParams, string &command, vector<string> &paramsVector);
bool readString(SOCKET& sock, string value);
// TODO: add headers that you want to pre-compile here

#endif //PCH_H
