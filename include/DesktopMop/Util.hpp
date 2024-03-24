#pragma once

#include <DesktopMop/Windows.hpp>
#include <string>

std::wstring StringToWString(std::string s);
std::string WStringToString(std::wstring s);

std::wstring WStringToLower(std::wstring ws);

bool IsElevated();

int CreateShellLink(LPCWSTR pathSrc, LPCWSTR pathDst, LPCWSTR description);