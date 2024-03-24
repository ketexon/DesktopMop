#pragma once

#include <DesktopMop/Windows.hpp>

enum class ControlIdentifier : WORD {
	StartStop = 100,
	WhitelistListView,
	BlacklistListView,
};

enum class MenuIdentifier : UINT_PTR {
	AddBlacklistItem = 1000,
	AddWhitelistItem,
	DeleteBlacklistItem,
	DeleteWhitelistItem,

	TrayApplicationName,
	TrayExitApplication,

	File,
	OpenDataFolder,
	Help,
	Close,
	Exit,
};

enum class AcceleratorIdentifier : WORD {
	Exit,
	Help
};

constexpr UINT kTrayIconID = 10'000;
constexpr UINT kTrayIconMessage = WM_USER + 1;