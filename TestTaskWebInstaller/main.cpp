#include <windows.h>
#include <commctrl.h>
#include <string>
#include <algorithm>
#include <vector>

#define ID_BUTTON_START 1001

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;
HWND g_hModeText = nullptr;

std::wstring g_currentMode = L"Не определен";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Функция для разбиения строки на аргументы
std::vector<std::wstring> ParseCommandLine(LPCWSTR cmdLine)
{
	std::vector<std::wstring> args;
	std::wstring arg;
	bool inQuotes = false;

	for (int i = 0; cmdLine[i] != L'\0'; i++) {
		if (cmdLine[i] == L'"') {
			inQuotes = !inQuotes;
		}
		else if (cmdLine[i] == L' ' && !inQuotes) {
			if (!arg.empty()) {
				args.push_back(arg);
				arg.clear();
			}
		}
		else {
			arg += cmdLine[i];
		}
	}

	if (!arg.empty()) {
		args.push_back(arg);
	}

	return args;
}

// Функция для определения режима по аргументам командной строки
std::wstring DetermineModeFromCommandLine()
{
	LPWSTR cmdLine = GetCommandLine();
	std::vector<std::wstring> args = ParseCommandLine(cmdLine);

	for (size_t i = 1; i < args.size(); i++) {
		std::wstring argLower = args[i];
		std::transform(argLower.begin(), argLower.end(), argLower.begin(), ::towlower);

		if (argLower == L"-wininet") {
			return L"Режим wininet";
		}
		else if (argLower == L"-curl") {
			return L"Режим curl";
		}
	}

	return L"Режим по умолчанию";
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	g_currentMode = DetermineModeFromCommandLine();
	const wchar_t CLASS_NAME[] = L"Main Window Class";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInst;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Тестовое окно",          
		WS_OVERLAPPED,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		500, 300,                     
		nullptr, nullptr, hInst, nullptr
	);

	if (hwnd == nullptr) {
		return 0; 
	}

	ShowWindow(hwnd, nCmdShow); 
	UpdateWindow(hwnd);

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	

	switch (uMsg) {

		case WM_CREATE:
		{
			std::wstring modeText = L"Режим работы: " + g_currentMode;

			g_hModeText = CreateWindow(L"STATIC", modeText.c_str(), WS_CHILD | WS_VISIBLE, 150, 30, 300, 25, hwnd, nullptr, nullptr, nullptr);
			g_hPercentText = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 50, 70, 200, 30, hwnd, nullptr, nullptr, nullptr);
			g_hProgress = CreateWindow(L"msctls_progress32", nullptr, WS_CHILD | WS_VISIBLE, 50, 100, 400, 30, hwnd, nullptr, nullptr, nullptr);	
			CreateWindow(L"BUTTON", L"Начать", WS_CHILD | WS_VISIBLE, 200, 150, 100, 25, hwnd, (HMENU)ID_BUTTON_START, nullptr, nullptr);
		
			return 0;
		}

		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);

			if (wmId == ID_BUTTON_START) 
			{				
				SetWindowText(g_hPercentText, L"Процесс запущен...");
				InvalidateRect(g_hPercentText, nullptr, TRUE);
				UpdateWindow(g_hPercentText);
				if (g_hProgress) {					
					SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
					for (int i = 0; i <= 100; i += 10) {
						SendMessage(g_hProgress, PBM_SETPOS, i, 0);
						Sleep(200);
						UpdateWindow(hwnd);
					}							
				}
				SetWindowText(g_hPercentText, L"Готово!");
				InvalidateRect(g_hPercentText, nullptr, TRUE);
				UpdateWindow(g_hPercentText);
			}	
			
			return 0;
		}

		case WM_DESTROY: 
		{
			PostQuitMessage(0);

			return 0;
		}		

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}