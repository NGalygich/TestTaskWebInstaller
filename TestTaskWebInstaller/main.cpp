#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "ui.h"
#include "download.h"
#include "stats.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib") 


HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
HWND g_hFolderEdit = nullptr;
std::wstring g_currentMode = L"";
wchar_t g_startTimeStr[50];
static HBRUSH g_hWhiteBrush = NULL;

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
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 350, nullptr, nullptr, hInst, nullptr);

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
		g_hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
		CreateUI(hwnd);
		return 0;
	}

	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hwnd, &rect);
		FillRect(hdc, &rect, g_hWhiteBrush);
		return 1;
	}

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetBkColor(hdcStatic, RGB(255, 255, 255));
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		return (LRESULT)g_hWhiteBrush;
	}

	case WM_COMMAND: {
		if (LOWORD(wParam) == ID_BUTTON_START) {
			SetWindowText(g_hPercentText, L"0%");
			SendMessage(g_hProgress, PBM_SETPOS, 0, 0);

			const char* url;
			if (IsDlgButtonChecked(hwnd, ID_RADIO_32) == BST_CHECKED) {
				url = "https://www.7-zip.org/a/7z2600.exe";
			}
			else {
				url = "https://www.7-zip.org/a/7z2600-x64.exe";
			}

			bool result;
			if (g_currentMode == L"Режим curl") {
				result = DownloadFileCurl(url);
			}
			else {
				wchar_t folderPath[MAX_PATH];
				GetWindowText(g_hFolderEdit, folderPath, MAX_PATH);
				wchar_t filePath[MAX_PATH];
				wsprintf(filePath, L"%s\\7-Zip.exe", folderPath);

				char filePathA[MAX_PATH];
				WideCharToMultiByte(CP_ACP, 0, filePath, -1, filePathA, MAX_PATH, NULL, NULL);

				result = DownloadFileWininet(url, filePathA);
			}

			SetWindowText(g_hPercentText, result ? L"Готово" : L"Ошибка");

			if (result) {
				SendMessage(g_hProgress, PBM_SETPOS, 100, 0);
				SetWindowText(g_hPercentText, L"Начинаю отправку статистики...");
				SendStats(g_startTimeStr, g_currentMode);
				SetWindowText(g_hPercentText, L"Готово");
			}
		}
		else if (LOWORD(wParam) == ID_BUTTON_BROWSE) {
			std::wstring folder = BrowseForFolder(hwnd);
			if (!folder.empty()) {
				SetWindowText(g_hFolderEdit, folder.c_str());
			}
		}
		else if (LOWORD(wParam) == ID_RADIO_32 || LOWORD(wParam) == ID_RADIO_64) {
			CheckRadioButton(hwnd, ID_RADIO_32, ID_RADIO_64, LOWORD(wParam));
		}
		return 0;
	}

	case WM_DESTROY: {
		if (g_hWhiteBrush) {
			DeleteObject(g_hWhiteBrush);
		}
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}