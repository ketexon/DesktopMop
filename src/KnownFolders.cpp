#include <DesktopMop/KnownFolders.hpp>

#include <DesktopMop/OnExitScope.hpp>

#include <iostream>

std::variant<KnownFolders, int> GetKnownFolders(){
	HRESULT hr;
	OnExitScope onExitScope;
	KnownFolders knownFolders;

	IKnownFolderManager* knownFolderManager = nullptr;
	hr = CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&knownFolderManager));
	if(FAILED(hr)){
		std::wcerr << L"Could not create KnownFolderManager object" << std::endl;
		return 1;
	}
	onExitScope.Add([&](){knownFolderManager->Release();});

	IKnownFolder* knownFolder = nullptr;
	LPWSTR knownFolderPath;

	hr = knownFolderManager->GetFolder(
		FOLDERID_Desktop,
		&knownFolder
	);
	if(FAILED(hr)){
		std::wcerr << L"Could not get Desktop folder" << std::endl;
		return 1;
	}

	hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &knownFolderPath);
	if(FAILED(hr)){
		std::wcerr << L"Could not get path to Desktop folder" << std::endl;
		return 1;
	}

	knownFolders.desktopFolder = knownFolderPath;

	knownFolder->Release();
	CoTaskMemFree(knownFolderPath);



	hr = knownFolderManager->GetFolder(
		FOLDERID_RoamingAppData,
		&knownFolder
	);
	if(FAILED(hr)){
		std::wcerr << L"Could not get AppData folder" << std::endl;
		return 1;
	}

	hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &knownFolderPath);
	if(FAILED(hr)){
		std::wcerr << L"Could not get path to AppData folder" << std::endl;
		return 1;
	}

	knownFolders.appDataFolder = knownFolderPath;

	knownFolder->Release();
	CoTaskMemFree(knownFolderPath);



	hr = knownFolderManager->GetFolder(
		FOLDERID_Startup,
		&knownFolder
	);
	if(FAILED(hr)){
		std::wcerr << L"Could not get Startup folder" << std::endl;
		return 1;
	}

	hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &knownFolderPath);
	if(FAILED(hr)){
		std::wcerr << L"Could not get path to Startup folder" << std::endl;
		return 1;
	}

	knownFolders.startupFolder = knownFolderPath;

	knownFolder->Release();
	CoTaskMemFree(knownFolderPath);



	hr = knownFolderManager->GetFolder(
		FOLDERID_StartMenu,
		&knownFolder
	);
	if(FAILED(hr)){
		std::wcerr << L"Could not get Start Menu folder" << std::endl;
		return 1;
	}

	hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &knownFolderPath);
	if(FAILED(hr)){
		std::wcerr << L"Could not get path to Start Menu folder" << std::endl;
		return 1;
	}

	knownFolders.startMenuFolder = knownFolderPath;

	knownFolder->Release();
	CoTaskMemFree(knownFolderPath);

	knownFolderManager->Release();

	return knownFolders;
}