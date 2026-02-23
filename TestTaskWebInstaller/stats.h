#ifndef STATS_H
#define STATS_H

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

bool SendStats(const std::wstring& startTime, const std::wstring& workMode);

#endif