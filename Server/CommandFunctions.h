#pragma once

//command functions
string pong(vector<string> params);
string shutdown_server(vector<string> params);
string additionCommand(vector<string> params);
string take_screenshot(vector<string> params);

int SaveScreenshot(string filename, ULONG uQuality); // by Napalm
int GetEncoderClsid(const wchar_t *format, CLSID *pClsid);

void splitRequestAndParams(string commandAndParams, string &command, vector<string> &paramsVector);