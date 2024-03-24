#pragma once

#include <ostream>
#include <iostream>

#define DESKTOPMOP_LOG_LEVEL_DEBUG 4
#define DESKTOPMOP_LOG_LEVEL_INFO 3
#define DESKTOPMOP_LOG_LEVEL_WARNING 2
#define DESKTOPMOP_LOG_LEVEL_ERROR 1
#define DESKTOPMOP_LOG_LEVEL_OFF 0

class NulStreambuf : public std::streambuf
{
private:
	char dummyBuffer[ 64 ];
protected:
    virtual int overflow( int c )
    {
        setp(dummyBuffer, dummyBuffer + sizeof(dummyBuffer));
        return (c == traits_type::eof()) ? '\0' : c;
    }
};

class NulOStream : private NulStreambuf, public std::ostream
{
public:
    NulOStream() : std::ostream( this ) {}
    NulStreambuf* rdbuf() const { return const_cast<NulStreambuf*>(static_cast<const NulStreambuf*>(this)); }
};

#define _WLOG_TRACE() << __FILE__ << L"::" << __FUNCTION__ << L"(" << __LINE__ << L"): "

#if DESKTOPMOP_LOG_LEVEL >= DESKTOPMOP_LOG_LEVEL_DEBUG
#define WLOG_DEBUG() std::wcout _WLOG_TRACE()
#else
#define WLOG_DEBUG() NulOStream{}
#endif

#if DESKTOPMOP_LOG_LEVEL >= DESKTOPMOP_LOG_LEVEL_INFO
#define WLOG_INFO() std::wcout _WLOG_TRACE()
#else
#define WLOG_INFO() NulOStream{}
#endif

#if DESKTOPMOP_LOG_LEVEL >= DESKTOPMOP_LOG_LEVEL_WARNING
#define WLOG_WARNING() std::wcout _WLOG_TRACE()
#else
#define WLOG_WARNING() NulOStream{}
#endif

#if DESKTOPMOP_LOG_LEVEL >= DESKTOPMOP_LOG_LEVEL_ERROR
#define WLOG_ERROR() std::wcerr _WLOG_TRACE()
#else
#define WLOG_ERROR() NulOStream{}
#endif

#if DESKTOPMOP_LOG_LEVEL >= DESKTOPMOP_LOG_LEVEL_ERROR
#define WLOG_ERROR() std::wcerr _WLOG_TRACE()
#else
#define WLOG_ERROR() NulOStream{}
#endif