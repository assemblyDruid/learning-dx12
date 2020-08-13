#pragma once

#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <windows.h>   // hresult

// STL
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

// Type short-hands
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define r32 float
#define r64 double

enum class LogType
{
    kInfo = 0,
    kWarning,
    kError
};

class LogManager
{
public:
    static void
    Log(const std::string& msg);

    static void
    Log(const std::string& msg, LogType log_type);
};

inline void
ThrowWithMessage(const char* msg = "")
{
    if (strlen(msg)) LogManager::Log(msg, LogType::kError);
    throw std::exception(msg);
}

inline void
ThrowWithMessage(HRESULT hr, const char* msg = "")
{
    std::stringstream error_message;
    error_message << "HRESULT: 0x" << std::hex << hr << std::dec << std::endl
                  << msg << std::endl
                  << std::flush;
    ThrowWithMessage(error_message.str().c_str());
}

inline void
Require(HRESULT hr)
{
    if (FAILED(hr)) { ThrowWithMessage(hr); }
}

inline void
Require(HRESULT hr, const char* msg)
{
    if (FAILED(hr)) { ThrowWithMessage(hr, msg); }
}

inline void
Require(bool cond, const char* msg)
{
    if (!cond) { ThrowWithMessage(msg); }
}

inline void
Assert(bool cond, const char* msg)
{
#if __DXDEBUG__ == 1
    Require(cond, msg);
#endif   // __DXDEBUG__ == 1
}
