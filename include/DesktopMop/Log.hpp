#pragma once

#include <ostream>
#include <iostream>

#define DESKTOPMOP_LOG_LEVEL_DEBUG 4
#define DESKTOPMOP_LOG_LEVEL_INFO 3
#define DESKTOPMOP_LOG_LEVEL_WARNING 2
#define DESKTOPMOP_LOG_LEVEL_ERROR 1
#define DESKTOPMOP_LOG_LEVEL_OFF 0

#if DESKTOPMOP_DEBUG
#define _WLOG_TRACE << __FILE__ << L"::" << __FUNCTION__ << L"(" << __LINE__ << L"): "
#else
#define _WLOG_TRACE
#endif

#define WLOG_DEBUG if constexpr(DESKTOPMOP_LOG_LEVEL < DESKTOPMOP_LOG_LEVEL_DEBUG) {} else std::wcout _WLOG_TRACE
#define WLOG_INFO if constexpr(DESKTOPMOP_LOG_LEVEL < DESKTOPMOP_LOG_LEVEL_INFO) {} else std::wcout _WLOG_TRACE
#define WLOG_WARNING if constexpr(DESKTOPMOP_LOG_LEVEL < DESKTOPMOP_LOG_LEVEL_WARNING) {} else std::wcout _WLOG_TRACE
#define WLOG_ERROR if constexpr(DESKTOPMOP_LOG_LEVEL < DESKTOPMOP_LOG_LEVEL_ERROR) {} else std::wcout _WLOG_TRACE