#pragma once

#include <DesktopMop/Windows.hpp>

#include <map>
#include <variant>
#include <string>

using Config = std::map<std::wstring, std::wstring>;

std::variant<Config, int> OpenConfig(HANDLE file);
int SaveConfig(HANDLE file, Config config);