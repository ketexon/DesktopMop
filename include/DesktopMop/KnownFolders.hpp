#pragma once

#include <string>
#include <variant>

struct KnownFolders {
	std::wstring appDataFolder;
	std::wstring startupFolder;
	std::wstring startMenuFolder;
	std::wstring desktopFolder;
};

std::variant<KnownFolders, int> GetKnownFolders();