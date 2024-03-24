#include <DesktopMop/Windows.hpp>
#include <DesktopMop/WindowState.hpp>

#include <iostream>

HWND WindowState::CreateListView(HWND hwnd, LPWSTR title, HMENU id, RECT rect){
	const HINSTANCE hinstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

	HWND listView = CreateWindowExW(
		LVS_EX_GRIDLINES,
		WC_LISTVIEWW,
		L"",
		WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_EDITLABELS | LVS_SINGLESEL | LVS_NOSORTHEADER,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		hwnd,
		id,
		hinstance,
		NULL
	);
	if(listView == NULL) return NULL;

	LVCOLUMNW column;
	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	column.iSubItem = 0;
	column.pszText = title;
	column.cx = 0;
	column.fmt = LVCFMT_LEFT;

	if(SendMessageW(listView, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&column)) == -1){
		std::wcout << L"Could not insert column into list view." << std::endl;
		DestroyWindow(listView);
		return NULL;
	}

	if(SendMessageW(listView, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE_USEHEADER) == FALSE){
		std::wcout << L"Could not set column width of list view." << std::endl;
		DestroyWindow(listView);
		return NULL;
	}

	return listView;
}



LRESULT WindowState::Paint(){
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
	EndPaint(hwnd, &ps);
	return 0;
}