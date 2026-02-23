#include "stats.h"
#include <string>
#include <vector>
#include <windows.h>

bool SendStats(const std::wstring& startTime, const std::wstring& workMode) {

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
        MessageBox(NULL, L"Не удалось создать запрос", L"Ошибка", MB_OK);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring jsonBody = L"{";
    jsonBody += L"\"startTime\":\"" + startTime + L"\",";
    jsonBody += L"\"workMode\":\"" + workMode + L"\",";
    jsonBody += L"\"elevationResult\":\"\",";
    jsonBody += L"\"downloadResult\":\"\"";
    jsonBody += L"}";

    std::string utf8Body = "";
    for (wchar_t c : jsonBody) {
        if (c < 128) { 
            utf8Body += (char)c;
        }
    }

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(hRequest,
        headers, wcslen(headers),
        (LPVOID)utf8Body.c_str(),
        utf8Body.length(),
        utf8Body.length(),
        0);

    if (!sent) {
        MessageBox(NULL, L"Не удалось отправить запрос", L"Ошибка", MB_OK);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL received = WinHttpReceiveResponse(hRequest, NULL);
    if (!received) {
        MessageBox(NULL, L"Не удалось получить ответ от сервера", L"Ошибка", MB_OK);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    BOOL queried = WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode, &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);

    if (queried) {
        wchar_t message[256];

        if (statusCode == 200) {
            wsprintf(message, L"Статистика отправлена!\nСервер ответил: %d OK", statusCode);
            MessageBox(NULL, message, L"Успех", MB_OK);
        }
        else if (statusCode == 400) {
            wsprintf(message, L"Ошибка: %d\nСервер не понял данные", statusCode);
            MessageBox(NULL, message, L"Ошибка", MB_OK);
        }
        else if (statusCode == 404) {
            wsprintf(message, L"Ошибка: %d\nАдрес /api/stats не найден", statusCode);
            MessageBox(NULL, message, L"Ошибка", MB_OK);
        }
        else {
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