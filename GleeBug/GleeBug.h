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

#include <windows.h>
#include <psapi.h>
#include <cstdint>

//macros
#define BIND(thisPtr, funcPtr) std::bind(&funcPtr, thisPtr)
#define BIND1(thisPtr, funcPtr) std::bind(&funcPtr, thisPtr, std::placeholders::_1)

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
        inline bool operator()(const Range & a, const Range & b) const //a before b?
        {
            return a.second < b.first;
        }
    };
}

#endif //GLEEBUG_H