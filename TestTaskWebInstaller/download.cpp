#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <curl/curl.h>
#include <cstdio>
#include "download.h"
#include "resource.h"

extern HWND g_hProgress;
extern HWND g_hPercentText;
extern HWND g_hFolderEdit;

size_t WriteDataCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
	return written;
}

int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	if (dltotal > 1024) {
		int percent = (int)((dlnow * 100) / dltotal);

		if (percent >= 0 && percent <= 100) {
			SendMessage(g_hProgress, PBM_SETPOS, percent, 0);

			wchar_t percentText[50];
			wsprintf(percentText, L"%d%%", percent);
			SetWindowText(g_hPercentText, percentText);
		}

		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

bool DownloadFileCurl(const char* url) {
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	wchar_t folderPath[MAX_PATH];
	GetWindowText(g_hFolderEdit, folderPath, MAX_PATH);

	wchar_t filePath[MAX_PATH];
	wsprintf(filePath, L"%s\\7-Zip.exe", folderPath);

	FILE* fp = nullptr;
	errno_t err = _wfopen_s(&fp, filePath, L"wb");
	if (err != 0 || fp == nullptr) {
		curl_easy_cleanup(curl);
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

	CURLcode res = curl_easy_perform(curl);

	fclose(fp);
	curl_easy_cleanup(curl);

	return (res == CURLE_OK);
}

bool DownloadFileWininet(const char* url, const char* outputFile) {
	HINTERNET hNet = InternetOpenA("Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hNet) return false;

	HINTERNET hUrl = InternetOpenUrlA(hNet, url, NULL, 0, 0, 0);
	if (!hUrl) {
		InternetCloseHandle(hNet);
		return false;
	}

	char contentLength[32] = { 0 };
	DWORD contentLengthSize = sizeof(contentLength);
	DWORD index = 0;
	int totalSize = 0;
	HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, contentLength, &contentLengthSize, &index);
	totalSize = atoi(contentLength);

	HANDLE hFile = CreateFileA(outputFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hNet);
		return false;
	}

	char buffer[1024];
	DWORD bytesRead;
	DWORD bytesWritten;
	int totalBytesRead = 0;

	SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

	while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
		totalBytesRead += bytesRead;

		if (totalSize > 0) {
			int percent = (totalBytesRead * 100) / totalSize;

			SendMessage(g_hProgress, PBM_SETPOS, percent, 0);

			wchar_t percentText[50];
			wsprintf(percentText, L"%d%%", percent);
			SetWindowText(g_hPercentText, percentText);

			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	CloseHandle(hFile);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hNet);
	return true;
}