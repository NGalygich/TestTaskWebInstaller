#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

bool SendStats(const std::wstring& startTime, const std::wstring& workMode,
    const std::wstring& elevationResult, const std::wstring& downloadResult,
    const std::wstring& downloadError);