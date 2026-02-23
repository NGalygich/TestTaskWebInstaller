#ifndef UI_H
#define UI_H

#include <windows.h>
#include <shlobj.h> 
#include <string>

std::wstring BrowseForFolder(HWND hwnd);
void CreateUI(HWND hwnd);

#endif