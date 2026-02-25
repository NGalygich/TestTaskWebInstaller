#include "stats.h"
#include <string>
#include <vector>
#include <windows.h>

bool SendStats(const std::wstring& startTime, const std::wstring& workMode,
    const std::wstring& elevationResult, const std::wstring& downloadResult,
    const std::wstring& downloadError, const std::wstring& launchResult) {

    HINTERNET hSession = WinHttpOpen(L"Simple Client",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession == NULL) {
        MessageBox(NULL, L"Не удалось создать сессию", L"Ошибка", MB_OK);
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 7268, 0);
    if (hConnect == NULL) {
        MessageBox(NULL, L"Не удалось подключиться к серверу. Запустите сервер!", L"Ошибка", MB_OK);
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/stats",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (hRequest == NULL) {       
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring downloadResultBool = (downloadResult == L"true") ? L"true" : L"false";
    std::wstring launchResultBool = (launchResult == L"true") ? L"true" : L"false";

    std::wstring jsonBody = L"{";
    jsonBody += L"\"startTime\":\"" + startTime + L"\",";
    jsonBody += L"\"workMode\":\"" + workMode + L"\",";
    jsonBody += L"\"elevationResult\":\"" + elevationResult + L"\",";
    jsonBody += L"\"downloadResult\":" + downloadResultBool + L",";
    jsonBody += L"\"downloadError\":\"" + downloadError + L"\",";
    jsonBody += L"\"launchResult\":" + launchResultBool;
    jsonBody += L"}";

    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, jsonBody.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8Body(utf8Size, 0);
    WideCharToMultiByte(CP_UTF8, 0, jsonBody.c_str(), -1, &utf8Body[0], utf8Size, NULL, NULL);

    utf8Body.pop_back();

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    if (!WinHttpSendRequest(hRequest, headers, wcslen(headers),
        (LPVOID)utf8Body.c_str(), utf8Body.length(), utf8Body.length(), 0)) {        
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        MessageBox(NULL, L"Не удалось получить ответ от сервера", L"Ошибка", MB_OK);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    if (WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode, &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX)) {

        wchar_t message[256];

        if (statusCode == 400) {
            wsprintf(message, L"Ошибка: %d\nСервер не понял данные", statusCode);
            MessageBox(NULL, message, L"Ошибка", MB_OK);
        }
        else if (statusCode == 404) {
            wsprintf(message, L"Ошибка: %d\nАдрес /api/stats не найден", statusCode);
            MessageBox(NULL, message, L"Ошибка", MB_OK);
        }
        else if (statusCode != 200) {
            wsprintf(message, L"Сервер вернул код: %d", statusCode);
            MessageBox(NULL, message, L"Ответ сервера", MB_OK);
        }
    }
    else {
        MessageBox(NULL, L"Не удалось получить код ответа", L"Предупреждение", MB_OK);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}