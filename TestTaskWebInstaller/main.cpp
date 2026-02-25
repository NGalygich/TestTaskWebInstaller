#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "ui.h"
#include "download.h"
#include "stats.h"
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib") 

#define WM_UPDATE_PROGRESS (WM_USER + 1)
#define WM_UPDATE_STATUS   (WM_USER + 2)

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
HWND g_hFolderEdit = nullptr;
std::wstring g_currentMode;
wchar_t g_startTimeStr[50];
static HBRUSH g_hWhiteBrush = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DWORD WINAPI DownloadThreadProc(LPVOID lpParam);

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
		CW_USEDEFAULT, CW_USEDEFAULT, 550, 350, nullptr, nullptr, hInst, nullptr);

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
            
            CreateThread(NULL, 0, DownloadThreadProc, hwnd, 0, NULL);
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

    case WM_UPDATE_PROGRESS:
    {
        SendMessage(g_hProgress, PBM_SETPOS, wParam, 0);
        return 0;
    }

    case WM_UPDATE_STATUS:
    {
        SetWindowText(g_hPercentText, (LPCWSTR)lParam);
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

DWORD WINAPI DownloadThreadProc(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;

    bool is64bit = IsDlgButtonChecked(hwnd, ID_RADIO_64) == BST_CHECKED;

    // скачивание с тетового сервера
    const char* url = is64bit
        ? "http://localhost:7268/download/7zip/64"
        : "http://localhost:7268/download/7zip/32";

    // если нужно скачать с оф. сайта 7-zip - заменить на этот вариант.
    //const char* url = is64bit
    //    ? "https://www.7-zip.org/a/7z2600-x64.exe"
    //    : "https://www.7-zip.org/a/7z2600.exe";


    std::wstring downloadResult;
    std::wstring downloadError;
    std::wstring elevationResult;
    std::wstring launchResult;
    bool downloadSuccess = false;
    bool serverAvailable = true;
    bool fileOnServer = true;

    wchar_t folderPath[MAX_PATH];
    GetWindowText(g_hFolderEdit, folderPath, MAX_PATH);
    wchar_t filePath[MAX_PATH];
    wsprintf(filePath, L"%s\\7-Zip.exe", folderPath);

    PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Подключение к серверу...");

    if (DeleteFile(filePath))
    {
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Старый файл удален");
    }
    else
    {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
        {
            PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Не удалось удалить старый файл");
        }
    }

    PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Подключение к серверу...");

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
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Ошибка сети. Повторная попытка через 2 секунды...");
        Sleep(2000); 

        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Повторная попытка...");

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
    }

    if (!downloadSuccess)
    {
        serverAvailable = false;
        fileOnServer = false;

        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Не удалось скачать с сервера. Загружаю из ресурсов...");

        downloadSuccess = CheckAndExtractFromResource(filePath, is64bit);

        if (downloadSuccess)
        {
            downloadResult = L"false";
            downloadError = L"файл загружен из ресурсов";
        }
    }
    else
    {
        PostMessage(hwnd, WM_UPDATE_PROGRESS, 100, 0);
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Файл скачан с сервера.");

        downloadResult = L"true";
        downloadError = L"";
    }

    if (downloadSuccess)
    {
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Запускаю файл...");

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
    }

    PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)L"Отправка статистики...");
    bool statsSent = SendStats(g_startTimeStr, g_currentMode, elevationResult, downloadResult, downloadError, launchResult);

    if (!statsSent)
    {
        MessageBox(hwnd, L"Не удалось отправить статистику на сервер", L"Ошибка", MB_OK | MB_ICONERROR);
    }

    if (downloadSuccess)
    {
        if (serverAvailable && fileOnServer)
        {
            PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)(statsSent
                ? L"Готово. Файл получен с сервера. Статистика отправлена."
                : L"Готово. Файл получен с сервера. Ошибка отправки статистики"));
        }
        else
        {
            PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)(statsSent
                ? L"Готово. Файл получен из ресурсов. Статистика отправлена."
                : L"Готово. Файл получен из ресурсов. Ошибка отправки статистики"));
        }
    }
    else
    {
        PostMessage(hwnd, WM_UPDATE_STATUS, 0, (LPARAM)(statsSent
            ? L"Файл не получен. Статистика отправлена."
            : L"Файл не получен. Ошибка отправки статистики"));
    }

    return 0;
}