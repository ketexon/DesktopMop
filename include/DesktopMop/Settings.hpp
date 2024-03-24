#pragma once

#include <string>
#include <optional>

struct Settings {
	bool console;
	std::optional<std::wstring> desktopFolder;
};