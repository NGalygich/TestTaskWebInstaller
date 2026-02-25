#define CURL_STATICLIB

#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <curl/curl.h>
#include <cstdio>
#include "download.h"
#include "resource.h" 


#ifndef WM_UPDATE_PROGRESS
#define WM_UPDATE_PROGRESS (WM_USER + 1)
#define WM_UPDATE_STATUS   (WM_USER + 2)
#endif

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
            HWND hMainWnd = GetParent(g_hProgress);
            PostMessage(hMainWnd, WM_UPDATE_PROGRESS, percent, 0);

            wchar_t percentText[50];
            wsprintf(percentText, L"%d%%", percent);
            PostMessage(hMainWnd, WM_UPDATE_STATUS, 0, (LPARAM)percentText);
        }
    }
    return 0;
}


bool DownloadFileCurl(const char* url) {
    static bool init = false;
    if (!init) {
        curl_global_init(CURL_GLOBAL_ALL);
        init = true;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    wchar_t folderPath[MAX_PATH];
    GetWindowText(g_hFolderEdit, folderPath, MAX_PATH);
    wchar_t filePath[MAX_PATH];
    wsprintf(filePath, L"%s\\7-Zip.exe", folderPath);

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, filePath, L"wb") != 0 || !fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        DeleteFileW(filePath);
        return false;
    }

    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    if (fileSize < 1024) {
        DeleteFileW(filePath);
        return false;
    }

    return true;
}

bool DownloadFileWininet(const char* url, const char* outputFile) {
    HINTERNET hNet = InternetOpenA("Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hNet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hNet, url, NULL, 0, 0, 0);
    if (!hUrl) {
        InternetCloseHandle(hNet);        
        HWND hMainWnd = GetParent(g_hProgress);
        PostMessage(hMainWnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Сервер не отвечает. Загружаю из ресурсов...");
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);

    if (statusCode != 200) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hNet);       
        HWND hMainWnd = GetParent(g_hProgress);
        PostMessage(hMainWnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Файл не найден на сервере. Загружаю из ресурсов...");
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

            HWND hMainWnd = GetParent(g_hProgress);
            PostMessage(hMainWnd, WM_UPDATE_PROGRESS, percent, 0);

            wchar_t percentText[50];
            wsprintf(percentText, L"%d%%", percent);
            PostMessage(hMainWnd, WM_UPDATE_STATUS, 0, (LPARAM)percentText);
        }
    }

    CloseHandle(hFile);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);

    if (totalBytesRead == 0) {
        DeleteFileA(outputFile);
        return false;
    }

    return true;
}

bool ExtractResourceToFile(int resourceId, const wchar_t* filePath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) {
        MessageBox(NULL, L"FindResource: ресурс не найден!", L"Ошибка", MB_OK);
        return false;
    }

    HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
    if (!hLoadedResource) {
        MessageBox(NULL, L"LoadResource: не удалось загрузить!", L"Ошибка", MB_OK);
        return false;
    }

    void* pResourceData = LockResource(hLoadedResource);
    DWORD resourceSize = SizeofResource(NULL, hResource);

    FILE* fp = nullptr;
    errno_t err = _wfopen_s(&fp, filePath, L"wb");
    if (err != 0 || fp == nullptr) {
        MessageBox(NULL, L"Не удалось создать файл!", L"Ошибка", MB_OK);
        return false;
    }

    size_t written = fwrite(pResourceData, 1, resourceSize, fp);
    fclose(fp);

    return (written == resourceSize);
}

bool CheckAndExtractFromResource(const wchar_t* filePath, bool is64bit) {
    FILE* checkFile = nullptr;
    errno_t err = _wfopen_s(&checkFile, filePath, L"rb");
    if (err == 0 && checkFile) {
        fclose(checkFile);
        return true;
    }

    int resourceId = is64bit ? IDR_7ZIP64 : IDR_7ZIP32;
    return ExtractResourceToFile(resourceId, filePath);
}