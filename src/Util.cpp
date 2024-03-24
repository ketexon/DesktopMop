#include <DesktopMop/Util.hpp>
#include <DesktopMop/Windows.hpp>
#include <DesktopMop/OnExitScope.hpp>
#include <DesktopMop/Log.hpp>

#include <iostream>

std::wstring StringToWString(std::string s){
	int wstringSize = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), NULL, 0);
	std::wstring wstring;
	wstring.resize(wstringSize);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), wstring.data(), wstringSize);
	return wstring;
}

std::string WStringToString(std::wstring ws){
	int stringSize = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.length(), NULL, NULL, NULL, NULL);
	std::string string;
	string.resize(stringSize);
	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.length(), string.data(), stringSize, NULL, NULL);
	return string;
}

std::wstring WStringToLower(std::wstring ws){
	const size_t size = ws.length() + 1;
	wchar_t* buffer = new wchar_t[size];
	wcscpy_s(buffer, size, ws.c_str());
	_wcslwr_s(buffer, size);
	std::wstring out{buffer};
	delete[] buffer;
	return out;
}

bool IsElevated(){
    bool elevated = false;
    HANDLE token = NULL;
    if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)){
        TOKEN_ELEVATION elevation;
		DWORD elevationSize;
        if(GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &elevationSize)) {
            elevated = elevation.TokenIsElevated;
        }
    }
    if(token) {
        CloseHandle(token);
    }
    return elevated;
}

int CreateShellLink(LPCWSTR pathSrc, LPCWSTR pathDst, LPCWSTR description){
	HRESULT hr;
	OnExitScope onExitScope;

	IShellLinkW* shellLink;
	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&shellLink));
	if(FAILED(hr)){
		WLOG_ERROR << L"Could not create CLSID_ShellLink instance" << std::endl;
		return 1;
	}
	onExitScope.Add([&](){shellLink->Release();});

	shellLink->SetPath(pathSrc);
	shellLink->SetDescription(description);

	IPersistFile* file;

	hr = shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&file));
	if(FAILED(hr)){
		WLOG_ERROR << L"Could not get IID_IPersistFile from shell link" << std::endl;
		return 2;
	}

	hr = file->Save(pathDst, TRUE);
	if(FAILED(hr)){
		WLOG_ERROR << L"Could not save shell link file." << std::endl;
		return 3;
	}

	file->Release();
	return 0;
}