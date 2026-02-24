#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "ui.h"
#include "download.h"
#include "stats.h"
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib") 

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
HWND g_hFolderEdit = nullptr;
std::wstring g_currentMode;
wchar_t g_startTimeStr[50];
static HBRUSH g_hWhiteBrush = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::wstring DetermineModeFromCommandLine()
{
	LPWSTR cmdLine = GetCommandLine();

	if (wcsstr(cmdLine, L"-wininet"))
		return L"wininet";
	if (wcsstr(cmdLine, L"-curl"))
		return L"curl";

	return L"wininet";
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE /*hPrevInst*/, LPSTR /*lpCmdLine*/, int nCmdShow)
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

	HWND hwnd = CreateWindowEx(0, L"MainClass", L"Загрузчик",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
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

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_BUTTON_START)
        {
            ShowWindow(g_hProgress, SW_SHOW);
            ShowWindow(g_hPercentText, SW_SHOW);

            SetWindowText(g_hPercentText, L"0%");
            SendMessage(g_hProgress, PBM_SETPOS, 0, 0);

            bool is64bit = IsDlgButtonChecked(hwnd, ID_RADIO_64) == BST_CHECKED;
            const char* url = is64bit
                ? "http://localhost:7268/download/7zip/64"
                : "http://localhost:7268/download/7zip/32";

            std::wstring downloadResult;
            std::wstring downloadError;
            std::wstring elevationResult;
            std::wstring launchResult;
            bool downloadSuccess = false;

            wchar_t folderPath[MAX_PATH];
            GetWindowText(g_hFolderEdit, folderPath, MAX_PATH);
            wchar_t filePath[MAX_PATH];
            wsprintf(filePath, L"%s\\7-Zip.exe", folderPath);

            if (g_currentMode == L"curl")
            {
                downloadSuccess = DownloadFileCurl(url);
            }
            else
            {
                char filePathA[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, filePath, -1, filePathA, MAX_PATH, NULL, NULL);
                downloadSuccess = DownloadFileWininet(url, filePathA);
            }

            if (!downloadSuccess)
            {
                SetWindowText(g_hPercentText, L"Сервер недоступен. Извлекаю из ресурсов...");
                downloadSuccess = CheckAndExtractFromResource(filePath, is64bit);
            }

            if (downloadSuccess)
            {
                SendMessage(g_hProgress, PBM_SETPOS, 100, 0);
                SetWindowText(g_hPercentText, L"Файл готов. Запускаю...");

                SHELLEXECUTEINFO sei = { sizeof(sei) };
                sei.lpVerb = L"runas";
                sei.lpFile = filePath;
                sei.nShow = SW_SHOWNORMAL;
                sei.fMask = SEE_MASK_DEFAULT;

                if (ShellExecuteEx(&sei))
                {
                    elevationResult = L"разрешена";
                    launchResult = L"true";
                }
                else
                {
                    DWORD error = GetLastError();
                    if (error == ERROR_CANCELLED)
                    {
                        elevationResult = L"отклонена";
                        launchResult = L"true";
                        ShellExecute(NULL, L"open", filePath, NULL, NULL, SW_SHOWNORMAL);
                    }
                    else
                    {
                        elevationResult = L"ошибка";
                        launchResult = L"false";
                    }
                }

                downloadResult = L"true";
                downloadError = L"";
            }
            else
            {
                SetWindowText(g_hPercentText, L"Ошибка. Файл не получен.");
                downloadResult = L"false";
                downloadError = L"Ошибка при получении файла";
                elevationResult = L"";
                launchResult = L"";
            }

            SetWindowText(g_hPercentText, L"Отправляю статистику...");
            bool statsSent = SendStats(g_startTimeStr, g_currentMode, elevationResult, downloadResult, downloadError, launchResult);

            if (downloadSuccess)
            {
                SetWindowText(g_hPercentText, statsSent
                    ? L"Готово. Файл получен. Статистика отправлена."
                    : L"Файл получен. Ошибка отправки статистики");
            }
            else
            {
                SetWindowText(g_hPercentText, statsSent
                    ? L"Файл не получен. Статистика отправлена."
                    : L"Файл не получен. Ошибка отправки статистики");
            }
        }
		else if (LOWORD(wParam) == ID_BUTTON_BROWSE)
		{
			std::wstring folder = BrowseForFolder(hwnd);
			if (!folder.empty())
			{
				SetWindowText(g_hFolderEdit, folder.c_str());
			}
		}
		else if (LOWORD(wParam) == ID_RADIO_32 || LOWORD(wParam) == ID_RADIO_64)
		{
			CheckRadioButton(hwnd, ID_RADIO_32, ID_RADIO_64, LOWORD(wParam));
		}
		return 0;
	}

	case WM_DESTROY:
	{
		if (g_hWhiteBrush)
		{
			DeleteObject(g_hWhiteBrush);
			g_hWhiteBrush = nullptr;
		}
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}