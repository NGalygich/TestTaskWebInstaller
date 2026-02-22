#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <winhttp.h> 
#include <fstream>  
#include <curl/curl.h>

#pragma comment(lib, "libcurl_imp.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")

#define ID_BUTTON_START 1001

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
std::wstring g_currentMode = L"";
wchar_t g_startTimeStr[50];

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::wstring DetermineModeFromCommandLine()
{
	LPWSTR cmdLine = GetCommandLine();

	if (wcsstr(cmdLine, L"-wininet"))
		return L"Режим wininet";
	if (wcsstr(cmdLine, L"-curl"))
		return L"Режим curl";

	return L"Режим по умолчанию";
}

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

bool DownloadFileCurl() {
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, "7-Zip.exe", "wb");
	if (err != 0 || fp == nullptr) {
		curl_easy_cleanup(curl);
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.7-zip.org/a/7z2600-x64.exe");
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

	PostMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS };
	InitCommonControlsEx(&icex);

	g_currentMode = DetermineModeFromCommandLine();

	SYSTEMTIME st;
	GetLocalTime(&st);
	wsprintf(g_startTimeStr, L"%04d-%02d-%02dT%02d:%02d:%02d",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInst;
	wc.lpszClassName = L"MainClass";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, L"MainClass", L"Загрузчик", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 300, nullptr, nullptr, hInst, nullptr);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: {
		std::wstring modeText = L"Режим: " + g_currentMode;
		CreateWindow(L"STATIC", modeText.c_str(), WS_CHILD | WS_VISIBLE, 150, 30, 300, 25, hwnd, nullptr, nullptr, nullptr);
		g_hPercentText = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 50, 70, 100, 30, hwnd, nullptr, nullptr, nullptr);
		g_hProgress = CreateWindow(PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE, 50, 100, 400, 30, hwnd, nullptr, nullptr, nullptr);
		CreateWindow(L"BUTTON", L"Скачать", WS_CHILD | WS_VISIBLE, 200, 150, 100, 25, hwnd, (HMENU)ID_BUTTON_START, nullptr, nullptr);
		return 0;
	}

	case WM_COMMAND: {
		if (LOWORD(wParam) == ID_BUTTON_START) {
			SetWindowText(g_hPercentText, L"0%");
			PostMessage(g_hProgress, PBM_SETPOS, 0, 0);

			bool result = (g_currentMode == L"Режим curl") ?
				DownloadFileCurl() :
				DownloadFileWininet("https://www.7-zip.org/a/7z2600-x64.exe", "7-Zip.exe");

			SetWindowText(g_hPercentText, result ? L"Готово" : L"Ошибка");
			if (result) PostMessage(g_hProgress, PBM_SETPOS, 100, 0);
			//SendStats(g_startTimeStr, g_currentMode);
		}
		return 0;
	}

	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}