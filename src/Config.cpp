#include <DesktopMop/Config.hpp>
#include <DesktopMop/WindowState.hpp>
#include <DesktopMop/Util.hpp>
#include <DesktopMop/Log.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>

std::variant<Config, int> OpenConfig(HANDLE file){
	DWORD fileSizeHigh;
	DWORD fileSizeLow = GetFileSize(file, &fileSizeHigh);
	LPARAM fileSize = MAKELPARAM(fileSizeLow, fileSizeHigh);

	std::string buffer;
	buffer.resize(fileSize);

	DWORD bytesRead;
	if(ReadFile(file, buffer.data(), fileSize, &bytesRead, NULL) == FALSE){
		WLOG_ERROR << L"Could not open file. Last error: " << GetLastError() << std::endl;
		return 1;
	}

	Config config;
	auto startIt = buffer.begin();
	int i = 1;
	while(true){
		// size_t endPos = buffer.find("\r\n", startIt - buffer.begin());
		auto endIt = std::find(startIt, buffer.end(), '\n');
		// auto endIt = endPos == buffer.npos ? buffer.end() : buffer.begin() + endPos;
		std::string lineUTF8{startIt, endIt};
		std::wstring line = StringToWString(lineUTF8);

		size_t lineStart = line.find_first_not_of(L' ');
		size_t lineEnd = line.find_last_not_of(L' ');
		line = line.substr(
			lineStart == line.npos ? 0 : lineStart,
			lineEnd == line.npos ? std::numeric_limits<size_t>::max() : lineEnd + 1
		);


		if (line.length() != 0) {
			size_t eqPos = line.find('=');
			if (eqPos == line.npos){
				std::wcout << L"Config file: Line " << i << L": " << std::quoted(line) << L" does not have a key" << std::endl;
				return 1;
			}
			std::wstring key = line.substr(0, eqPos);
			size_t keyEnd = key.find_last_not_of(L' ');
			key = key.substr(0, keyEnd == key.npos ? std::numeric_limits<size_t>::max() : keyEnd + 1);
			key = WStringToLower(key);

			std::wstring value = line.substr(eqPos + 1);
			size_t valueStart = value.find_first_not_of(L' ');
			value = value.substr(valueStart == value.npos ? 0 : valueStart);

			config[key] = value;
		}

		if(endIt == buffer.end()) break;
		startIt = endIt + 1;
		++i;
	}

	return config;
}


int SaveConfig(HANDLE file, Config config){
	std::stringstream ss;
	for(const auto& [k, v] : config){
		ss << WStringToString(k) << '=' << WStringToString(v) << '\n';
	}
	std::string content = ss.str();
	if(WriteFile(file, content.c_str(), content.size(), NULL, NULL) == FALSE){
		std::wcout << L"Could not save config (" << GetLastError() << L")" << std::endl;
		return 1;
	}
	return 0;
}

constexpr wchar_t kFileListDelim = L'\30'; // record separator

void WindowState::LoadConfig(){
	WLOG_DEBUG << L"Loading config" << std::endl;
	if(config.contains(L"blacklisted")){
		std::wstring blacklistedStr = config.at(L"blacklisted");
		while(blacklistedStr.size() > 0){
			WLOG_DEBUG << L"Config contains blacklisted files." << std::endl;

			size_t delimPos = blacklistedStr.find(kFileListDelim);
			std::wstring entry;
			if(delimPos == blacklistedStr.npos){
				entry = blacklistedStr;
				blacklistedStr.clear();
			}
			else{
				entry = blacklistedStr.substr(0, delimPos);
				blacklistedStr.erase(blacklistedStr.begin(), blacklistedStr.begin() + delimPos + 1);
			}

			if(entry.size() > 0){
				TryAddBlackListedFile(entry);
			}
		}
	}

	if(config.contains(L"whitelisted")){
		std::wstring whitelistedStr = config.at(L"whitelisted");
		while(whitelistedStr.size() > 0){
			WLOG_DEBUG << L"Config contains whitelisted files." << std::endl;

			size_t delimPos = whitelistedStr.find(kFileListDelim);
			std::wstring entry;
			if(delimPos == whitelistedStr.npos){
				entry = whitelistedStr;
				whitelistedStr.clear();
			}
			else{
				entry = whitelistedStr.substr(0, delimPos);
				whitelistedStr.erase(whitelistedStr.begin(), whitelistedStr.begin() + delimPos + 1);
			}

			if(entry.size() > 0){
				TryAddWhiteListedFile(entry);
			}
		}
	}
}

void WindowState::SaveConfig(){
	std::wstringstream blackListedFilesSS, whiteListedFilesSS;
	for(size_t i = 0; i < blackListedFiles.size(); ++i){
		blackListedFilesSS << blackListedFiles.at(i);
		if(i != blackListedFiles.size() - 1) blackListedFilesSS << kFileListDelim;
	}

	for(size_t i = 0; i < whiteListedFiles.size(); ++i){
		whiteListedFilesSS << whiteListedFiles.at(i);
		if(i != whiteListedFiles.size() - 1) whiteListedFilesSS << kFileListDelim;
	}

	config[L"blacklisted"] = blackListedFilesSS.str();
	config[L"whitelisted"] = whiteListedFilesSS.str();

	WLOG_DEBUG << L"Saving config..." << std::endl;

	HANDLE file = CreateFileW(configFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file != INVALID_HANDLE_VALUE){
		::SaveConfig(file, config);
		CloseHandle(file);
	}
	else{
		WLOG_WARNING << L"Could not open config file (" << GetLastError() << L")" << std::endl;
	}
}