#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <string>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "comctl32.lib")

#define ID_BUTTON_START 1001

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
std::wstring g_currentMode = L"";

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

bool DownloadFileCurl() {
	return false; // Заглушка для режима curl, реализацию можно добавить позже
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
			PostMessage(g_hProgress, PBM_SETPOS, percent, 0);

			wchar_t percentText[50];
			wsprintf(percentText, L"%d%%", percent);
			SetWindowText(g_hPercentText, percentText);
		}
	}

	CloseHandle(hFile);
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hNet);
	return true;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS };
	InitCommonControlsEx(&icex);

	g_currentMode = DetermineModeFromCommandLine();

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