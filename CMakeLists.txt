cmake_minimum_required(VERSION 3.25)

project(DesktopMop LANGUAGES CXX RC)

set(
	DESKTOPMOP_SOURCES
	Main.cpp
	Settings.hpp
	WindowState.hpp WindowState.cpp
	FileFilter.cpp

	KnownFolders.cpp KnownFolders.hpp
	Config.hpp Config.cpp
	Util.hpp Util.cpp

	ID.hpp

	UI.cpp

	Log.hpp

	OnExitScope.hpp OnExitScope.cpp

	Windows.hpp

	Const.hpp Const.cpp
)

set(
	DESKTOPMOP_PC_HEADERS
	Windows.hpp
)

set(
	DESKTOPMOP_RESOURCES
	rc.rc
)

message(${DESKTOPMOP_SOURCES})

list(TRANSFORM DESKTOPMOP_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/" REGEX ^.*\\.cpp$)
list(TRANSFORM DESKTOPMOP_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/include/DesktopMop/" REGEX ^.*\\.hpp$)

list(TRANSFORM DESKTOPMOP_PC_HEADERS PREPEND "${CMAKE_SOURCE_DIR}/include/DesktopMop/")

list(TRANSFORM DESKTOPMOP_RESOURCES PREPEND "${CMAKE_SOURCE_DIR}/rc/")

add_executable(
	DesktopMop WIN32
	${DESKTOPMOP_SOURCES}
	${DESKTOPMOP_RESOURCES}
)

target_link_libraries(
	DesktopMop
	Shlwapi
	Comctl32
)

target_include_directories(
	DesktopMop
	PUBLIC include
)

target_precompile_headers(
	DesktopMop
	PUBLIC
	${DESKTOPMOP_PC_HEADERS}
)

set_target_properties(
	DesktopMop
	PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED TRUE
)

target_compile_definitions(
	DesktopMop
	PUBLIC
	DESKTOPMOP_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
	DESKTOPMOP_SHOW_CONSOLE=$<IF:$<CONFIG:Debug>,1,0>
	DESKTOPMOP_CLEAR_CONFIG=0
	DESKTOPMOP_LOG_LEVEL=4
	UNICODE
	_UNICODE
	NOMINMAX
)