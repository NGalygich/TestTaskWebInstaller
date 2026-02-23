#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <windows.h>
#include <wininet.h>
#include <curl/curl.h>
#include <cstdio>

#pragma comment(lib, "libcurl_imp.lib")
#pragma comment(lib, "wininet.lib")

size_t WriteDataCallback(void* ptr, size_t size, size_t nmemb, FILE* stream);
int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
bool DownloadFileCurl(const char* url);
bool DownloadFileWininet(const char* url, const char* outputFile);

#endif