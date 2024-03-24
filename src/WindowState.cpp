#include <DesktopMop/WindowState.hpp>

#include <DesktopMop/Windows.hpp>
#include <DesktopMop/OnExitScope.hpp>
#include <DesktopMop/Log.hpp>
#include <DesktopMop/Const.hpp>
#include <DesktopMop/ID.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

bool WindowState::LockMutex(){
	DWORD dr = WaitForSingleObject(windowStateMutex, INFINITE);
	if(dr != WAIT_OBJECT_0 && dr != WAIT_ABANDONED){
		WLOG_ERROR << L"Wait for window state mutex failed. Last Error: " << GetLastError() << std::endl;
		PostMessageW(hwnd, WM_CLOSE, 1, NULL);
		return false;
	}
	return true;
}

void WindowState::ReleaseMutex(){
	::ReleaseMutex(windowStateMutex);
}

bool WindowState::TryAddListedFile(
	std::vector<std::wstring>& targetFileList,
	std::vector<std::wregex>& targetRegexList,
	std::wstring newValue
){
	targetFileList.push_back(newValue);
	try {
		targetRegexList.emplace_back(newValue, std::regex_constants::icase);
		return true;
	}
	catch(std::regex_error e){
		return false;
	}
}

bool WindowState::TryUpdateListedFile(
	std::vector<std::wstring>& targetFileList,
	std::vector<std::wregex>& targetRegexList,
	size_t index,
	std::wstring newValue
){
	targetFileList.at(index) = newValue;
	try {
		targetRegexList.at(index) = std::wregex{newValue, std::regex_constants::icase};
		return true;
	}
	catch(std::regex_error e){
		return false;
	}
}

void WindowState::DeleteListedFile(
	std::vector<std::wstring>& targetFileList,
	std::vector<std::wregex>& targetRegexList,
	size_t index
){
	targetFileList.erase(targetFileList.begin() + index);
	targetRegexList.erase(targetRegexList.begin() + index);
}

bool WindowState::TryAddBlackListedFile(std::wstring v){return TryAddListedFile(blackListedFiles, blackListedRegex, v);}
bool WindowState::TryAddWhiteListedFile(std::wstring v){return TryAddListedFile(whiteListedFiles, whiteListedRegex, v);}
bool WindowState::TryUpdateBlackListedFile(size_t index, std::wstring v){return TryUpdateListedFile(blackListedFiles, blackListedRegex, index, v);}
bool WindowState::TryUpdateWhiteListedFile(size_t index, std::wstring v){return TryUpdateListedFile(whiteListedFiles, whiteListedRegex, index, v);}
void WindowState::DeleteBlackListedFile(size_t index){return DeleteListedFile(blackListedFiles, blackListedRegex, index);}
void WindowState::DeleteWhiteListedFile(size_t index){return DeleteListedFile(whiteListedFiles, whiteListedRegex, index);}

std::optional<LRESULT> WindowState::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch(uMsg){
		case WM_CREATE: return Create();
		case WM_PAINT: return Paint();
		case WM_COMMAND: {
			// menu/accelerator
			if(lParam == 0){
				if(HIWORD(wParam) == 0) return HandleMenuCommand(LOWORD(wParam));
				else if(HIWORD(wParam) == 1) return HandleAcceleratorCommand(LOWORD(wParam));
			}
			else{
				return HandleControlCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
			}
		}
		case WM_NOTIFY: return HandleNotify(reinterpret_cast<NMHDR*>(lParam));
		case kTrayIconMessage: return HandleTrayIconMessage(HIWORD(lParam), LOWORD(lParam), GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam));
		case WM_CLOSE: return Close();
		case WM_DESTROY: return Destroy();
	}
	return std::nullopt;
}

LRESULT WindowState::Create(){
	HRESULT hr;
	const HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if(FAILED(hr)){
		std::wcerr << L"Could not initialize COM. " << std::endl;
		return 1;
	}


	auto knownFoldersVariant = GetKnownFolders();
	// if returned string
	if(const KnownFolders* knownFolders = std::get_if<KnownFolders>(&knownFoldersVariant)){
		this->knownFolders = *knownFolders;
	}
	// else (if err)
	else{
		return std::get<int>(knownFoldersVariant);
	}

	if(settings.desktopFolder){
		knownFolders.desktopFolder = *settings.desktopFolder;
	}

	wchar_t desktopGlob[MAX_PATH];
	if(PathCombineW(desktopGlob, knownFolders.desktopFolder.c_str(), L"*") == NULL){
		WLOG_ERROR << L"PathCombineW failed. Last Error: " << GetLastError() << std::endl;
		PostMessageW(hwnd, WM_CLOSE, 1, NULL);
	}
	this->desktopGlob = desktopGlob;

	int ir = InitializeFiles();
	if(ir != 0){
		return ir;
	}

	ir = InitializeFirstOpen();
	if(ir != 0){
		return ir;
	}

	LoadConfig();

	windowStateMutex = CreateMutexW(NULL, FALSE, L"WindowStateMutex");
	if(windowStateMutex == NULL){
		std::wcerr << L"CreateMutexW returned NULL" << std::endl;
		return 1;
	}

	/*---- Create UI ----*/
	{
		InitCommonControls();
		INITCOMMONCONTROLSEX initCommonControls;
		initCommonControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		initCommonControls.dwICC = ICC_LISTVIEW_CLASSES;
		if(InitCommonControlsEx(&initCommonControls) == FALSE){
			std::wcerr << L"Could not initialize common controls" << std::endl;
			return 1;
		}
	}

	// Start/stop button
	HWND button = CreateWindowExW(
		0,
		WC_BUTTONW,
		L"Start",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHLIKE | BS_AUTOCHECKBOX | BS_VCENTER,
		10,
		10,
		100,
		32,
		hwnd,
		reinterpret_cast<HMENU>(ControlIdentifier::StartStop),
		hinstance,
		NULL
	);
	if(button == NULL){
		std::wcerr << L"Could not create button" << std::endl;
		return 1;
	}

	// Set Icon
	HICON icon = LoadIconW(hinstance, L"ICON");

	SetClassLongPtr(
		hwnd,
		GCLP_HICON,
		reinterpret_cast<LONG_PTR>(icon)
	);

	// Add tray icon
	{
		NOTIFYICONDATAW notifyIcon;
		notifyIcon.cbSize = sizeof(NOTIFYICONDATAW);
		notifyIcon.hWnd = hwnd;
		notifyIcon.uID = kTrayIconID;
		notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON;
		notifyIcon.uCallbackMessage = kTrayIconMessage;
		notifyIcon.hIcon = icon;
		notifyIcon.uVersion = NOTIFYICON_VERSION_4;

		Shell_NotifyIconW(NIM_ADD, &notifyIcon);
		Shell_NotifyIconW(NIM_SETVERSION, &notifyIcon);
	}

	// List view UI
	std::wstring listViewTitle = L"Blacklisted Files";

	constexpr int listViewPadding = 10;
	int listViewOffset = 50;
	const int listViewHeight = (kWindowSafeHeight - listViewOffset - listViewPadding)/2;

	blacklistListView = CreateListView(
		hwnd,
		listViewTitle.data(),
		reinterpret_cast<HMENU>(ControlIdentifier::BlacklistListView),
		RECT{
			10, listViewOffset,
			kWindowWidth - 20, listViewOffset + listViewHeight
		}
	);
	listViewOffset += listViewHeight + listViewPadding;
	if(blacklistListView == NULL){
		std::wcerr << L"Could not create list view" << std::endl;
		return 1;
	}

	listViewTitle = L"Whitelisted Files";
	whitelistListView = CreateListView(
		hwnd,
		listViewTitle.data(),
		reinterpret_cast<HMENU>(ControlIdentifier::WhitelistListView),
		RECT{
			10, listViewOffset,
			kWindowWidth - 20, listViewOffset + listViewHeight
		}
	);
	if(whitelistListView == NULL){
		std::wcerr << L"Could not create list view" << std::endl;
		return 1;
	}

	{
		LVITEM item;
		item.mask = LVIF_STATE | LVIF_TEXT;
		item.iSubItem = 0;
		item.state = 0;

		for(size_t i = 0; i < blackListedFiles.size(); ++i){
			item.iItem = i;
			item.pszText = blackListedFiles.at(i).data();

			const int newIdx = SendMessageW(blacklistListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
			if(newIdx == -1){
				WLOG_ERROR << L"Could not insert item into blacklist view (" << GetLastError() << L")" << std::endl;
				PostMessageW(hwnd, WM_CLOSE, 1, NULL);
			}
		}

		for(size_t i = 0; i < whiteListedFiles.size(); ++i){
			item.iItem = i;
			item.pszText = whiteListedFiles.at(i).data();

			const int newIdx = SendMessageW(whitelistListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
			if(newIdx == -1){
				WLOG_ERROR << L"Could not insert item into whitelist view (" << GetLastError() << L")" << std::endl;
				PostMessageW(hwnd, WM_CLOSE, 1, NULL);
			}
		}
	}


	// Register change notification
	changeNotification = FindFirstChangeNotificationW(
		knownFolders.desktopFolder.c_str(),
		FALSE,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
	);

	if(changeNotification == INVALID_HANDLE_VALUE){
		std::wcerr << L"FindFirstChangeNotificationW returned invalid handle" << std::endl;
		return 1;
	}

	changeNotificationMutex = CreateMutexW(NULL, FALSE, L"ChangeNotificationMutex");
	if(changeNotificationMutex == NULL){
		std::wcerr << L"CreateMutexW returned NULL" << std::endl;
		return 1;
	}

	BOOL br = RegisterWaitForSingleObject(
		&changeNotificationCallback,
		changeNotification,
		::DesktopChangeNotification,
		static_cast<PVOID>(this),
		INFINITE,
		WT_EXECUTEDEFAULT | WT_EXECUTELONGFUNCTION
	);

	if(br == FALSE){
		std::wcerr << L"RegisterWaitForSingleObject returned 0" << std::endl;
		return 1;
	}
	return 0;
}

LRESULT WindowState::Destroy(){
	if(windowStateMutex){
		LockMutex();
	}
	if(changeNotificationCallback){
		UnregisterWait(changeNotificationCallback);
	}
	if(changeNotification){
		FindCloseChangeNotification(changeNotification);
	}
	if(changeNotificationMutex){
		CloseHandle(changeNotificationMutex);
	}
	if(acceleratorTable){
		DestroyAcceleratorTable(acceleratorTable);
	}

	{
		NOTIFYICONDATAW notifyIcon;
		notifyIcon.cbSize = sizeof(NOTIFYICONDATAW);
		notifyIcon.hWnd = hwnd;
		notifyIcon.uID = kTrayIconID;
		notifyIcon.uFlags = 0;

		Shell_NotifyIconW(NIM_DELETE, &notifyIcon);
	}

	SaveConfig();

	CoUninitialize();
	PostQuitMessage(0);

	ReleaseMutex();
	return 0;
}

LRESULT WindowState::Close(){
	ShowWindow(hwnd, SW_HIDE);
	return 0;
}


std::optional<LRESULT> WindowState::HandleMenuCommand(WORD menuId){
	if(menuId == static_cast<WORD>(MenuIdentifier::AddBlacklistItem) || menuId == static_cast<WORD>(MenuIdentifier::AddWhitelistItem)){
		if(!LockMutex()) return std::nullopt;

		WLOG_DEBUG << L"Add item" << std::endl;
		HWND targetHWnd =
			menuId == static_cast<WORD>(MenuIdentifier::AddBlacklistItem)
			? blacklistListView
			: whitelistListView;

		auto targetAddFunction =
			menuId == static_cast<WORD>(MenuIdentifier::AddBlacklistItem)
			? &WindowState::TryAddBlackListedFile
			: &WindowState::TryAddWhiteListedFile;

		int itemIdx = SendMessageW(targetHWnd, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
		if(itemIdx == -1){
			itemIdx = SendMessageW(targetHWnd, LVM_GETITEMCOUNT, 0, 0);
		}
		else{
			++itemIdx;
		}

		wchar_t itemText[256];
		StringCbCopyW(itemText, 256, L"filepath");

		LVITEM item;
		item.mask = LVIF_STATE | LVIF_TEXT;
		item.iItem = itemIdx;
		item.iSubItem = 0;
		item.state = 0;
		item.pszText = itemText;

		const int newIdx = SendMessageW(targetHWnd, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
		if(newIdx == -1){
			WLOG_ERROR << L"Could not insert item.  " << GetLastError() << std::endl;
			PostMessageW(hwnd, WM_CLOSE, 1, NULL);
			return std::nullopt;
		}
		(this->*targetAddFunction)(itemText);

		SendMessageW(targetHWnd, LVM_EDITLABELW, newIdx, 0);

		ReleaseMutex();
		return 0;
	}
	else if(menuId == static_cast<WORD>(MenuIdentifier::DeleteBlacklistItem) || menuId == static_cast<WORD>(MenuIdentifier::DeleteWhitelistItem)){
		if(!LockMutex()) return std::nullopt;;

		WLOG_DEBUG << L"Delete item" << std::endl;
		HWND targetHWnd =
			menuId == static_cast<WORD>(MenuIdentifier::DeleteBlacklistItem)
				? this->blacklistListView
				: this->whitelistListView;

		auto targetDeleteFunction =
			menuId == static_cast<WORD>(MenuIdentifier::DeleteBlacklistItem)
				? &WindowState::DeleteBlackListedFile
				: &WindowState::DeleteWhiteListedFile;

		int itemIdx = SendMessageW(targetHWnd, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
		if(itemIdx != -1){
			SendMessageW(targetHWnd, LVM_DELETEITEM, itemIdx, 0);

			(this->*targetDeleteFunction)(static_cast<size_t>(itemIdx));
		}

		ReleaseMutex();
		return 0;
	}
	else if(menuId == static_cast<WORD>(MenuIdentifier::TrayExitApplication) || menuId == static_cast<WORD>(MenuIdentifier::Exit)){
		PostQuitMessage(0);
		return 0;
	}
	else if(menuId == static_cast<WORD>(MenuIdentifier::Close)){
		ShowWindow(hwnd, SW_HIDE);
		return 0;
	}
	else if(menuId == static_cast<WORD>(MenuIdentifier::OpenDataFolder)){
		wchar_t dir[MAX_PATH];
		PathCombineW(dir, knownFolders.appDataFolder.c_str(), kAppDataName);

		WLOG_DEBUG << dir << std::endl;

		HINSTANCE hr = ShellExecuteW(NULL, L"explore", dir, NULL, NULL, SW_NORMAL);
		if(hr <= reinterpret_cast<HINSTANCE>(32)){
			WLOG_WARNING << L"Could not open explorer (" << hr << L", " << GetLastError() << L")" << std::endl;
		}
	}
	return std::nullopt;
}

std::optional<LRESULT> WindowState::HandleAcceleratorCommand(WORD id){
	switch(static_cast<AcceleratorIdentifier>(id)){
		case AcceleratorIdentifier::Exit: {
			PostQuitMessage(0);
			return 0;
		}
	}
	return std::nullopt;
}

std::optional<LRESULT> WindowState::HandleControlCommand(WORD id, WORD code, HWND controlHWnd){
	if(id = static_cast<WORD>(ControlIdentifier::StartStop) && code == BN_CLICKED){
		if(!LockMutex()) return std::nullopt;

		if(SendMessageW(controlHWnd, BM_GETCHECK, 0, 0) == BST_CHECKED){
			started = true;
			SendMessageW(controlHWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"Stop"));
		}
		else{
			started = false;
			SendMessageW(controlHWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"Start"));
		}

		ReleaseMutex();
	}
	return std::nullopt;
}

std::optional<LRESULT> WindowState::HandleNotify(NMHDR* nm){
	if(nm->idFrom == static_cast<UINT_PTR>(ControlIdentifier::BlacklistListView) || nm->idFrom == static_cast<UINT_PTR>(ControlIdentifier::WhitelistListView)){
		if(nm->code == NM_DBLCLK){
			NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(nm);
			// clicked on background
			if(itemActivate->iItem == -1) return 0;

			HWND edit = reinterpret_cast<HWND>(SendMessageW(nm->hwndFrom, LVM_EDITLABELW, itemActivate->iItem, 0));
			if(edit == NULL){
				WLOG_ERROR << L"Could not edit label: " << GetLastError() << std::endl;
				PostMessageW(hwnd, WM_CLOSE, 1, NULL);
				return std::nullopt;
			}
		}
		else if(nm->code == LVN_ENDLABELEDITW){
			if(!LockMutex()) return std::nullopt;

			const NMLVDISPINFOW* dispInfo = reinterpret_cast<NMLVDISPINFOW*>(nm);

			BOOL result = TRUE;

			// user edits text
			if(dispInfo->item.pszText != NULL){
				auto targetEditFunction =
					nm->idFrom == static_cast<UINT_PTR>(ControlIdentifier::BlacklistListView)
					? &WindowState::TryUpdateBlackListedFile
					: &WindowState::TryUpdateWhiteListedFile;
				bool br = (this->*targetEditFunction)(dispInfo->item.iItem, dispInfo->item.pszText);
				if(br == false){
					std::wstringstream mbText;
					mbText << L"Regex input " << std::quoted(dispInfo->item.pszText) << L" is invalid.\nIf you need help with regex, go to File->Help.";
					MessageBoxW(hwnd, mbText.str().c_str(), L"Invalid Regex", MB_OK | MB_ICONERROR);
					result = FALSE;
				}
			}

			ReleaseMutex();
			return result;
		}
		else if(nm->code == NM_RCLICK){
			NMITEMACTIVATE* itemActivate = reinterpret_cast<NMITEMACTIVATE*>(nm);
			POINT screenPoint = itemActivate->ptAction;
			ClientToScreen(nm->hwndFrom, &screenPoint);

			HMENU menu = CreatePopupMenu();
			AppendMenuW(
				menu,
				MF_STRING,
				nm->idFrom == static_cast<UINT_PTR>(ControlIdentifier::BlacklistListView)
					? static_cast<UINT_PTR>(MenuIdentifier::AddBlacklistItem)
					: static_cast<UINT_PTR>(MenuIdentifier::AddWhitelistItem),
				L"Add"
			);

			// clicked on item
			if(itemActivate->iItem != -1){
				AppendMenuW(
					menu,
					MF_STRING,
					nm->idFrom == static_cast<UINT_PTR>(ControlIdentifier::BlacklistListView)
						? static_cast<UINT_PTR>(MenuIdentifier::DeleteBlacklistItem)
						: static_cast<UINT_PTR>(MenuIdentifier::DeleteWhitelistItem),
					L"Delete"
				);
			}

			TrackPopupMenu(
				menu,
				TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
				screenPoint.x,
				screenPoint.y,
				0,
				hwnd,
				NULL
			);
		}
	}
	return 0;
}

LRESULT WindowState::HandleTrayIconMessage(WORD id, WORD code, int x, int y){
	if(id == kTrayIconID){
		if(code == WM_CONTEXTMENU){
			HMENU menu = CreatePopupMenu();
			AppendMenuW(menu, MF_DISABLED | MF_STRING, static_cast<UINT_PTR>(MenuIdentifier::TrayApplicationName), kWindowName);
			AppendMenuW(menu, MF_SEPARATOR, NULL, NULL);
			AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuIdentifier::TrayExitApplication), L"E&xit");
			TrackPopupMenu(
				menu,
				TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
				x, y,
				0,
				hwnd,
				NULL
			);
		}
		else if(code == NIN_SELECT || code == NIN_KEYSELECT){
			if(IsWindowVisible(hwnd) == FALSE){
				ShowWindow(hwnd, SW_SHOW);
			}
		}
	}
	return 0;
}

void WindowState::CreateAccelerators(){
	std::vector<ACCEL> accelerators {
		ACCEL {
			FCONTROL | FVIRTKEY, 'Q',
			static_cast<WORD>(AcceleratorIdentifier::Exit)
		},
		ACCEL {
			FVIRTKEY, VK_F1,
			static_cast<WORD>(AcceleratorIdentifier::Help)
		},
	};

	acceleratorTable = CreateAcceleratorTableW(accelerators.data(), accelerators.size());
	if(acceleratorTable == NULL){
		WLOG_ERROR << L"Could not create accelerator table (" << GetLastError() << L")" << std::endl;
	}
}