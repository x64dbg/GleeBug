#ifndef GLEEBUG_H
#define GLEEBUG_H

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <algorithm>

#include <windows.h>
#include <psapi.h>
#include <cstdint>

//macros
#define BIND(thisPtr, funcPtr) std::bind(&funcPtr, thisPtr)
#define BIND1(thisPtr, funcPtr) std::bind(&funcPtr, thisPtr, std::placeholders::_1)

#ifdef _WIN64
#define X64DBG_MOD L"x64dbg.dll"
#else
#define X64DBG_MOD L"x32dbg.dll"
#endif //_WIN64

template<typename... Args>
inline void dprintf(const char *fmt, Args... args)
{
    static auto fn = (int(*)(const char* format, ...))GetProcAddress(GetModuleHandleW(X64DBG_MOD), "_plugin_logprintf");
    if (!fn)
        fn = printf;
    fn(fmt, args...);
}

#ifdef _WIN64
#define GleeArchValue(x32value, x64value) x64value
#else
#define GleeArchValue(x32value, x64value) x32value
#endif //_WIN64

namespace GleeBug
{
    typedef int8_t int8;
    typedef uint8_t uint8;
    typedef int16_t int16;
    typedef uint16_t uint16;
    typedef int32_t int32;
    typedef uint32_t uint32;
    typedef int64_t int64;
    typedef uint64_t uint64;
#ifdef _WIN64
    typedef uint64 ptr;
#else //x32
    typedef uint32 ptr;
#endif //_WIN64

    typedef std::pair<ptr, ptr> Range;

    /**
    \brief A range compare (used in std::map).
    */
    struct RangeCompare
    {
        /**
        \brief Returns if range a comes before range b.
        \param a First range.
        \param b Second range.
        \return True if a comes before b, false otherwise.
        */
        bool operator()(const Range & a, const Range & b) const //a before b?
        {
            return a.second < b.first;
        }
    };
}

#endif //GLEEBUG_H
