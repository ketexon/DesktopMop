#pragma once

#include <DesktopMop/Windows.hpp>
#include <DesktopMop/Settings.hpp>
#include <DesktopMop/KnownFolders.hpp>
#include <DesktopMop/Config.hpp>

#include <optional>
#include <string>
#include <vector>
#include <regex>

VOID CALLBACK DesktopChangeNotification(_In_ PVOID lpParam, _In_ BOOLEAN fired);

struct WindowState {
	HANDLE windowStateMutex = NULL;
	HACCEL acceleratorTable = NULL;

	Settings settings;
	Config config;
	HWND hwnd;

	bool started = false;
	KnownFolders knownFolders;
	std::wstring exeLocation;
	std::wstring configFilePath;
	std::wstring desktopGlob;
	HANDLE changeNotification = NULL;
	HANDLE changeNotificationCallback = NULL;
	HANDLE changeNotificationMutex = NULL;

	HWND blacklistListView = NULL;
	HWND whitelistListView = NULL;

	std::vector<std::wstring> blackListedFiles;
	std::vector<std::wregex> blackListedRegex;

	std::vector<std::wstring> whiteListedFiles;
	std::vector<std::wregex> whiteListedRegex;

	bool TryUpdateListedFile(
		std::vector<std::wstring>& targetFileList,
		std::vector<std::wregex>& targetRegexList,
		size_t index,
		std::wstring newValue
	);

	bool TryAddListedFile(
		std::vector<std::wstring>& targetFileList,
		std::vector<std::wregex>& targetRegexList,
		std::wstring newValue
	);

	void DeleteListedFile(
		std::vector<std::wstring>& targetFileList,
		std::vector<std::wregex>& targetRegexList,
		size_t index
	);

	bool TryUpdateBlackListedFile(size_t index, std::wstring newValue);
	bool TryUpdateWhiteListedFile(size_t index, std::wstring newValue);
	bool TryAddBlackListedFile(std::wstring newValue);
	bool TryAddWhiteListedFile(std::wstring newValue);
	void DeleteBlackListedFile(size_t index);
	void DeleteWhiteListedFile(size_t index);

	bool shouldElevate = false;

	static HWND CreateListView(HWND hwnd, LPWSTR title, HMENU id, RECT rect);

	bool LockMutex();
	void ReleaseMutex();

	int InitializeFiles();
	int InitializeFirstOpen();

	void LoadConfig();
	void SaveConfig();

	int PromptCreateExeShortcut(
		LPCWSTR path,
		LPCWSTR name,
		LPCWSTR title
	);

	void CleanDesktop();
	bool FilterDesktopFile(WIN32_FIND_DATAW* findData);

	std::optional<LRESULT> HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT Paint();
	LRESULT Create();
	LRESULT Close();
	LRESULT Destroy();

	void CreateAccelerators();

	std::optional<LRESULT> HandleMenuCommand(WORD id);
	std::optional<LRESULT> HandleAcceleratorCommand(WORD id);
	std::optional<LRESULT> HandleControlCommand(WORD id, WORD code, HWND controlHWnd);

	std::optional<LRESULT> HandleNotify(NMHDR* nm);

	LRESULT HandleTrayIconMessage(WORD id, WORD code, int x, int y);

	void DesktopChangeNotification(BOOLEAN fired);
};