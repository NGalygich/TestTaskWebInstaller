#include <windows.h>
#include <commctrl.h>
#include <shlobj.h> 
#include <string>
#include "ui.h"
#include "resource.h"

extern HWND g_hProgress;
extern HWND g_hPercentText;
extern HWND g_hFolderEdit;
extern std::wstring g_currentMode;

std::wstring BrowseForFolder(HWND hwnd) {
	BROWSEINFOW bi = { 0 };
	bi.hwndOwner = hwnd;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if (pidl != 0) {
		wchar_t path[MAX_PATH];
		if (SHGetPathFromIDListW(pidl, path)) {
			IMalloc* imalloc = 0;
			if (SUCCEEDED(SHGetMalloc(&imalloc))) {
				imalloc->Free(pidl);
				imalloc->Release();
			}
			return std::wstring(path);
		}
	}
	return L"";
}

void CreateUI(HWND hwnd) {
	std::wstring modeText = L"Режим: " + g_currentMode;
	CreateWindowW(L"STATIC", modeText.c_str(), WS_CHILD | WS_VISIBLE, 50, 30, 300, 25, hwnd, nullptr, nullptr, nullptr);

	CreateWindowW(L"STATIC", L"Папка для сохранения:", WS_CHILD | WS_VISIBLE, 50, 60, 400, 20, hwnd, nullptr, nullptr, nullptr);
	g_hFolderEdit = CreateWindowW(L"EDIT", L"C:\\", WS_CHILD | WS_VISIBLE | WS_BORDER, 50, 85, 300, 25, hwnd, (HMENU)ID_EDIT_FOLDER, nullptr, nullptr);
	CreateWindowW(L"BUTTON", L"Обзор", WS_CHILD | WS_VISIBLE, 360, 85, 80, 25, hwnd, (HMENU)ID_BUTTON_BROWSE, nullptr, nullptr);

	CreateWindowW(L"BUTTON", L"32-bit", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 50, 120, 80, 25, hwnd, (HMENU)ID_RADIO_32, nullptr, nullptr);
	CreateWindowW(L"BUTTON", L"64-bit", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 50, 145, 80, 25, hwnd, (HMENU)ID_RADIO_64, nullptr, nullptr);
	CheckRadioButton(hwnd, ID_RADIO_32, ID_RADIO_64, ID_RADIO_64);

	g_hPercentText = CreateWindowW(L"STATIC", L"", WS_CHILD, 50, 170, 400, 30, hwnd, nullptr, nullptr, nullptr);
	g_hProgress = CreateWindowW(PROGRESS_CLASS, nullptr, WS_CHILD | PBS_SMOOTH, 50, 200, 400, 30, hwnd, (HMENU)IDC_PROGRESS, nullptr, nullptr);

	CreateWindowW(L"BUTTON", L"Скачать", WS_CHILD | WS_VISIBLE, 200, 250, 100, 25, hwnd, (HMENU)ID_BUTTON_START, nullptr, nullptr);
}