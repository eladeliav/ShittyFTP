#include "pch.h"
#include "CommandFunctions.h"
#pragma warning(disable : 4996) //disable "unsafe function" warning

namespace fs = std::experimental::filesystem;

int GetEncoderClsid(const wchar_t *format, CLSID *pClsid)
{
	using namespace Gdiplus;
	unsigned int num = 0, size = 0;
	GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;
	ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *)(malloc(size));
	if (pImageCodecInfo == NULL) return -1;
	GetImageEncoders(num, size, pImageCodecInfo);

	for (unsigned int j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

int SaveScreenshot(string filename, ULONG uQuality) // by Napalm
{
	using namespace Gdiplus;
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	HWND hMyWnd = GetDesktopWindow();
	RECT r;
	int w, h;
	HDC dc, hdcCapture;
	int nBPP, nCapture, iRes;
	LPBYTE lpCapture;
	CLSID imageCLSID;
	Bitmap *pScreenShot;

	// get the area of my application's window     
	GetWindowRect(hMyWnd, &r);
	dc = GetWindowDC(hMyWnd);   // GetDC(hMyWnd) ;
	w = r.right - r.left;
	h = r.bottom - r.top;
	nBPP = GetDeviceCaps(dc, BITSPIXEL);
	hdcCapture = CreateCompatibleDC(dc);

	// create the buffer for the screenshot
	BITMAPINFO bmiCapture = { sizeof(BITMAPINFOHEADER), w, -h, 1, nBPP, BI_RGB, 0, 0, 0, 0, 0, };

	// create a container and take the screenshot
	HBITMAP hbmCapture = CreateDIBSection(dc, &bmiCapture, DIB_PAL_COLORS, (LPVOID *)&lpCapture, NULL, 0);

	// failed to take it
	if (!hbmCapture) {
		DeleteDC(hdcCapture);
		DeleteDC(dc);
		GdiplusShutdown(gdiplusToken);
		printf("failed to take the screenshot. err: %d\n", GetLastError());
		return 0;
	}

	// copy the screenshot buffer
	nCapture = SaveDC(hdcCapture);
	SelectObject(hdcCapture, hbmCapture);
	BitBlt(hdcCapture, 0, 0, w, h, dc, 0, 0, SRCCOPY);
	RestoreDC(hdcCapture, nCapture);
	DeleteDC(hdcCapture);
	DeleteDC(dc);

	// save the buffer to a file   
	pScreenShot = new Bitmap(hbmCapture, (HPALETTE)NULL);
	EncoderParameters encoderParams;
	encoderParams.Count = 1;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Guid = EncoderQuality;
	encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].Value = &uQuality;
	GetEncoderClsid(L"image/jpeg", &imageCLSID);

	wchar_t *lpszFilename = new wchar_t[filename.length() + 1];
	mbstowcs(lpszFilename, filename.c_str(), filename.length() + 1);

	iRes = (pScreenShot->Save(lpszFilename, &imageCLSID, &encoderParams) == Ok);
	delete pScreenShot;
	DeleteObject(hbmCapture);
	GdiplusShutdown(gdiplusToken);
	return iRes;
}

bool isNumber(const char* pStr)
{
	if (NULL == pStr || *pStr == '\0')
		return false;
	int dotCount = 0;
	int plusCount = 0;
	int minusCount = 0;

	while (*pStr)
	{
		char c = *pStr;
		switch (c)
		{
		case '.':
			if (++dotCount > 1)
				return false;
			break;
		case '-':
			if (++minusCount > 1)
				return false;
			break;
		case '+':
			if (++plusCount > 1)
				return false;
			break;
		default:
			if (c < '0' || c > '9')
				return false;
		}
		pStr++;
	}
	return true;
}

string take_screenshot(vector<string> params)
{
	POINT a, b;
	a.x = 0;
	a.y = 0;

	b.x = 1920;
	b.y = 1080;

	string path = "./screenshot.jpg";
	ULONG quality = 100;
	SaveScreenshot(path, quality);
	return "Screenshot take and save at " + path;
}

string pong(vector<string> params)
{
	return "Pong!";
}

string shutdown_server(vector<string> params)
{
	//running = false;
	return "Server Shutting Down";
}

string additionCommand(vector<string> params)
{
	if (!isNumber(params[0].c_str()) || !isNumber(params[1].c_str()))
		return "Wrong use of parameters. Both parameters must be digits";
	int a = std::stoi(params[0]);
	int b = std::stoi(params[1]);
	return std::to_string(a + b);
}

string directoryCommand(vector<string> params)
{
	string path = params[0];
	if (!dirExists(path))
		return "Directory not found";
	string toReturn = "\r\n";
	for (const auto & entry : fs::directory_iterator(path))
	{
		toReturn += entry.path().string() + "\r\n";
	}
	return toReturn;
}

bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

string deleteFileCommand(vector<string> params)
{
	FILE *filehandle = fopen(params[0].c_str(), "rb");
	if (filehandle == NULL)
	{
		return "File Not Found";
	}
	fclose(filehandle);
	if(remove(params[0].c_str()) != 0)
		return "Error deleting file";
	return "File deleted";
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