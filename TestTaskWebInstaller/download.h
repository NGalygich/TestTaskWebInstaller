#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#define CURL_STATICLIB

#include <windows.h>
#include <wininet.h>
#include <curl/curl.h>
#include <cstdio>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")      
#pragma comment(lib, "wldap32.lib")     
#pragma comment(lib, "crypt32.lib")     
#pragma comment(lib, "advapi32.lib") 

size_t WriteDataCallback(void* ptr, size_t size, size_t nmemb, FILE* stream);
int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
bool DownloadFileCurl(const char* url);
bool DownloadFileWininet(const char* url, const char* outputFile);

bool ExtractResourceToFile(int resourceId, const wchar_t* filePath);
bool CheckAndExtractFromResource(const wchar_t* filePath, bool is64bit);

#endif