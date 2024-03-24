#include <DesktopMop/Windows.hpp>
#include <DesktopMop/OnExitScope.hpp>
#include <DesktopMop/WindowState.hpp>

#include <iomanip>
#include <iostream>

void WindowState::CleanDesktop(){
	WIN32_FIND_DATAW findData;
	HANDLE findFileHandle = FindFirstFileW(desktopGlob.c_str(), &findData);
	if(findFileHandle == INVALID_HANDLE_VALUE){
		std::wcout << L"FindFirstFileW returned INVALID_HANDLE_VALUE. " << GetLastError() << std::endl;
		PostMessageW(hwnd, WM_CLOSE, 1, NULL);
	}

	FilterDesktopFile(&findData);

	while(FindNextFileW(findFileHandle, &findData) != FALSE){
		FilterDesktopFile(&findData);
	}
	if(GetLastError() != ERROR_NO_MORE_FILES){
		std::wcout << L"FindNextFileW returned 0 with error: " << GetLastError() << std::endl;
		PostMessageW(hwnd, WM_CLOSE, 1, NULL);
	}
}

VOID CALLBACK DesktopChangeNotification(_In_ PVOID lpParam, _In_ BOOLEAN fired){
	auto* const state = reinterpret_cast<WindowState*>(lpParam);
	state->DesktopChangeNotification(fired);
}

void WindowState::DesktopChangeNotification(BOOLEAN fired){
	OnExitScope onExitScope;

	DWORD dr = WaitForSingleObject(changeNotificationMutex, 0);

	// Mutex is locked;
	if(dr == WAIT_OBJECT_0 || dr == WAIT_ABANDONED){
		onExitScope.Add([&](){::ReleaseMutex(changeNotificationMutex);});
		if(!LockMutex()) return;

		FindNextChangeNotification(changeNotification);

		if(started) CleanDesktop();

		ReleaseMutex();
		return;
	}
	else if(dr == WAIT_FAILED){
		std::wcout << L"Wait failed. Last Error: " << GetLastError() << std::endl;
		PostMessageW(hwnd, WM_CLOSE, 1, NULL);
		return;
	}
}

bool WindowState::FilterDesktopFile(WIN32_FIND_DATAW* findData){
	std::wstring name {findData->cFileName};
	if(name == L"." || name == L".." || (findData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) > 0){
		return false;
	}
	std::wcout << findData->cFileName << L" " << std::hex << findData->dwFileAttributes << std::endl;
	// check if file is whitelisted
	for(size_t i = 0; i < whiteListedRegex.size(); ++i){
		const auto& re = whiteListedRegex.at(i);
		if(std::regex_match(findData->cFileName, re)){
			std::wcout << std::quoted(findData->cFileName)
				<< L" is whitelisted according to regex "
				<< std::quoted(whiteListedFiles.at(i))
				<< L". Not deleting."
				<< std::endl;
			return false;
		}
	}

	for(size_t i = 0; i < blackListedRegex.size(); ++i){
		const auto& re = blackListedRegex.at(i);
		if(std::regex_match(findData->cFileName, re)){
			std::wcout
				<< std::quoted(findData->cFileName)
				<< L" is blacklisted according to regex "
				<< std::quoted(blackListedFiles.at(i))
				<< L". Deleting."
				<< std::endl;

			wchar_t path[MAX_PATH + 1];
			PathCombineW(path, knownFolders.desktopFolder.c_str(), findData->cFileName);
			DeleteFileW(path);

			// notify that we deleted something so desktop will refresh
			SHChangeNotify(
				SHCNE_DELETE,
				SHCNF_PATH,
				path,
				NULL
			);

			return true;
		}
	}

	return false;
}