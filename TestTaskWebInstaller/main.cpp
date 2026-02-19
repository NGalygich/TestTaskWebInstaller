#include <windows.h>
#include <commctrl.h>

#define ID_BUTTON_START 1001

HWND g_hProgress = nullptr;
HWND g_hPercentText = nullptr;


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
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
			CreateWindow(L"STATIC", L"Режим работы", WS_CHILD | WS_VISIBLE, 300, 30, 100, 25, hwnd, nullptr, nullptr, nullptr);
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