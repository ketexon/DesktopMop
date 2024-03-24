#ifndef DESKTOPMOP_DEBUG
#define DESKTOPMOP_DEBUG 0
#endif

#include <DesktopMop/Windows.hpp>
#include <DesktopMop/WindowState.hpp>
#include <DesktopMop/Settings.hpp>
#include <DesktopMop/OnExitScope.hpp>
#include <DesktopMop/Config.hpp>
#include <DesktopMop/Util.hpp>
#include <DesktopMop/ID.hpp>
#include <DesktopMop/Log.hpp>
#include <DesktopMop/Const.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <variant>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitConsole(){
	AllocConsole();
	FILE *file;
	freopen_s(&file, "CONIN$", "r", stdin);
	freopen_s(&file, "CONOUT$", "w", stderr);
	freopen_s(&file, "CONOUT$", "w", stdout);
}

void PromptExit(){
	std::wcout << L"Press enter to continue . . .";
	std::wstring placeholder;
	std::getline(std::wcin, placeholder);
}

HMENU CreateWindowMenu(){
	const HMENU menu = CreateMenu();
	const HMENU fileSubMenu = CreateMenu();

	BOOL br;
	wchar_t title[256];

	MENUITEMINFOW breakItemInfo;
	breakItemInfo.cbSize = sizeof(MENUITEMINFOW);
	breakItemInfo.fMask = MIIM_FTYPE;
	breakItemInfo.fType = MFT_SEPARATOR;


	wcscpy_s(title, L"&File");

	MENUITEMINFOW menuItemInfo;
	menuItemInfo.cbSize = sizeof(MENUITEMINFOW);
	menuItemInfo.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_STRING;
	menuItemInfo.fType = MFT_STRING;
	menuItemInfo.fState = MFS_ENABLED;
	menuItemInfo.hSubMenu = fileSubMenu;
	menuItemInfo.dwTypeData = title;
	menuItemInfo.cch = 0;

	br = InsertMenuItemW(menu, 0, TRUE, &menuItemInfo);

	menuItemInfo.fMask |= MIIM_ID;
	menuItemInfo.fMask &= ~MIIM_SUBMENU;
	menuItemInfo.hSubMenu = NULL;

	wcscpy_s(title, L"&Open data folder");
	menuItemInfo.wID = static_cast<UINT>(MenuIdentifier::OpenDataFolder);
	br = InsertMenuItemW(fileSubMenu, 0, TRUE, &menuItemInfo);

	wcscpy_s(title, L"&Close\tAlt+F4");
	menuItemInfo.wID = static_cast<UINT>(MenuIdentifier::Close);
	br = InsertMenuItemW(fileSubMenu, 1, TRUE, &menuItemInfo);

	wcscpy_s(title, L"E&xit");
	menuItemInfo.wID = static_cast<UINT>(MenuIdentifier::Exit);
	br = InsertMenuItemW(fileSubMenu, 2, TRUE, &menuItemInfo);

	br = InsertMenuItemW(fileSubMenu, 1, TRUE, &breakItemInfo);

	return menu;
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	int nCmdShow
) {
	bool initConsole = static_cast<bool>(DESKTOPMOP_SHOW_CONSOLE);
	if(initConsole){
		InitConsole();
	}

	Settings settings {
		.console = initConsole
	};

	std::wstring exeLocation;
	std::vector<std::wstring> argv;
	{
		int nArgs;
		const LPWSTR* argvArr = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		exeLocation = *argvArr;
		++argvArr;
		--nArgs;
		while(nArgs-- > 0){
			argv.push_back(std::wstring{*(argvArr++)});
		}
	}

	for(size_t i = 0; i < argv.size(); ++i){
		const auto& arg = argv[i];
		if(arg == L"--console"){
			settings.console = true;
		}
		else if(arg == L"--hide-console"){
			settings.console = false;
		}
		else if(arg.starts_with(L"--desktop=")){
			std::wstring desktop = arg.substr(10);
			while(desktop.ends_with(L'/') || desktop.ends_with(L'\\') || desktop.ends_with(L'"')){
				desktop = desktop.substr(0, desktop.length() - 1);
			}
			DWORD attributes = GetFileAttributesW(desktop.c_str());
			if(attributes == INVALID_FILE_ATTRIBUTES){
				std::wcout << L"Could not get attributes for " << std::quoted(desktop) << L"\nMake sure the path points to a valid directory." << std::endl;
				PromptExit();
				return 1;
			}
			if((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0){
				std::wcout << std::quoted(desktop) << L" is not a directory." << std::endl;
				PromptExit();
				return 1;
			}
			settings.desktopFolder = desktop;
		}
	}

	if(settings.console && !initConsole) {
		InitConsole();
	}

	{
		WNDCLASSW wndClass = {
			.lpfnWndProc = WindowProc,
			.hInstance = hInstance,
			.lpszClassName = kClassName
		};

		if(RegisterClassW(&wndClass) == 0){
			std::wcerr << L"Could not register window class. Last Error: " << GetLastError() << std::endl;
			return 1;
		}
	}

	WindowState* windowState = new WindowState;
	windowState->settings = settings;
	windowState->exeLocation = exeLocation;

	windowState->CreateAccelerators();

	HMENU menu = CreateWindowMenu();

	HWND hwnd = CreateWindowExW(
		0,
		kClassName,
		kWindowName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		kWindowWidth, kWindowHeight,
		NULL,
		menu,
		hInstance,
		windowState
	);

	if(hwnd == NULL){
		std::wcerr << L"Could not create window. Last Error: " << GetLastError() << std::endl;
		return 2;
	}

	ShowWindow(hwnd, nCmdShow);

	MSG msg{};
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		if (!TranslateAcceleratorW(hwnd, windowState->acceleratorTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}

	DestroyWindow(hwnd);

	PromptExit();

	delete windowState;

	return 0;
}

int WindowState::InitializeFiles(){
	wchar_t path[MAX_PATH];
	if(PathCombineW(path, knownFolders.appDataFolder.c_str(), kAppDataName) == NULL){
		std::wcout << L"Could not combine app data folder to app data name" << std::endl;
		return 1;
	}

	if(!PathFileExistsW(path)){
		if(CreateDirectoryW(path, NULL) == FALSE){
			std::wcout << L"CreateDirectory failed. LastError: " << GetLastError() << std::endl;
		}
	}
	else if(PathIsDirectoryW(path) == FALSE){
		std::wcout << L"AppData directory name conflict with file. Please delete" << std::quoted(path) << std::endl;
		return 1;
	}

	if(PathCombineW(path, path, L"settings.cfg") == NULL){
		std::wcout << L"Could not combine app data folder settings.cfg" << std::endl;
		return 1;
	}

	configFilePath = path;

	HANDLE configFile = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(configFile == NULL){
		WLOG_WARNING() << L"Could not open settings.cfg" << std::endl;
		return 1;
	}

	if(!DESKTOPMOP_CLEAR_CONFIG){
		auto configVariant = OpenConfig(configFile);
		if(const auto configPointer = std::get_if<std::map<std::wstring, std::wstring>>(&configVariant)){
			config = *configPointer;
		}
		else{
			WLOG_WARNING() << L"Could not open config file (" << GetLastError() << L")" << std::endl;
			return 1;
		}
	}

	CloseHandle(configFile);

	return 0;
}

int WindowState::PromptCreateExeShortcut(
	LPCWSTR path,
	LPCWSTR name,
	LPCWSTR title
){
	std::wstringstream wss;
	wss << L"Create " << name;
	if(MessageBoxW(hwnd, wss.str().c_str(), title, MB_YESNO) == IDYES){
		bool shouldCreate = false;
		BOOL br;
		br = PathFileExistsW(path);
		if(br == TRUE){
			wss.str(L"");
			wss << name << L" already exists. Recreate it?";
			if(MessageBoxW(hwnd, wss.str().c_str(), title, MB_YESNO) == IDYES){
				br = DeleteFileW(path);
				if(br == FALSE){
					wss.str(L"");
					wss << L"Could not delete " << name;
					MessageBoxW(hwnd, wss.str().c_str(), title, MB_OK | MB_ICONERROR);
				}
				else{
					shouldCreate = true;
				}
			}
		}
		else{
			shouldCreate = true;
		}

		if(shouldCreate){
			int ir = CreateShellLink(exeLocation.c_str(), path, NULL);
			if(ir != 0){
				std::wcout << L"CreateShellLink failed" << std::endl;
				return 1;
			}
		}
	}
	return 0;
}

int WindowState::InitializeFirstOpen(){
	const auto it = config.find(L"initialized");
	if(it == config.end() || it->second != L"1"){
		wchar_t path[MAX_PATH];
		PathCombineW(path, knownFolders.startMenuFolder.c_str(), L"Programs/DesktopMop.lnk");
		PromptCreateExeShortcut(path, L"Start Menu shortcut", L"Start Menu");

		PathCombineW(path, knownFolders.desktopFolder.c_str(), L"DesktopMop.lnk");
		PromptCreateExeShortcut(path, L"Desktop shortcut", L"Desktop");

		PathCombineW(path, knownFolders.startupFolder.c_str(), L"DesktopMop.lnk");
		PromptCreateExeShortcut(path, L"Startup item", L"Startup");

		config[L"initialized"] = L"1";

		SaveConfig();
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	const HINSTANCE hinstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	WindowState* state = nullptr;
	HRESULT hr;

	if(uMsg == WM_NCCREATE){
		CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		state = reinterpret_cast<WindowState*>(pCreate->lpCreateParams);
		state->hwnd = hwnd;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	else{
		state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		std::optional<LRESULT> lrOpt = state->HandleMessage(uMsg, wParam, lParam);
		if(lrOpt) return *lrOpt;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}