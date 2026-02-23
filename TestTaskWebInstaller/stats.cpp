#include "stats.h"

bool SendStats(const std::wstring& startTime, const std::wstring& workMode) {
	HINTERNET hSession = WinHttpOpen(L"Stats Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
	if (!hSession) return false;

	HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 7268, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		return false;
	}

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/stats", NULL, NULL, NULL, 0);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	std::wstring jsonBody = L"{"
		L"\"startTime\":\"" + startTime + L"\","
		L"\"workMode\":\"" + workMode + L"\","
		L"\"elevationResult\":\"\"," // потом добавить
		L"\"downloadResult\":\"\"" // потом добавить
		L"}";

	std::string utf8Body = std::string(jsonBody.begin(), jsonBody.end());

	LPCWSTR headers = L"Content-Type: application/json\r\n";

	WinHttpSendRequest(hRequest, headers, wcslen(headers), (LPVOID)utf8Body.c_str(), utf8Body.length(), utf8Body.length(), 0);
	WinHttpReceiveResponse(hRequest, NULL);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return true;
}