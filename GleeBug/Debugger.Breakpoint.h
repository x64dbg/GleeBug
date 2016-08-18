#ifndef DEBUGGER_BREAKPOINT_H
#define DEBUGGER_BREAKPOINT_H

#include "Debugger.Global.h"

namespace GleeBug
{
    enum class BreakpointType
    {
        Software,
        Hardware,
        Memory
    };

    enum class SoftwareType
    {
        ShortInt3
    };

    enum class HardwareSlot
    {
        Dr0 = 0,
        Dr1 = 1,
        Dr2 = 2,
        Dr3 = 3
    };

    enum class HardwareType
    {
        Access,
        Write,
        Execute
    };

    enum class HardwareSize
    {
        SizeByte = 1,
        SizeWord = 2,
        SizeDword = 4,
#ifdef _WIN64
        SizeQword = 8
#endif //_WIN64
    };

    enum class MemoryType
    {
        Access = 1,
        Write = 2,
        Execute = 4
    };

    /**
    \brief Structure describing internal breakpoint info.
    */
    struct BreakpointInternalInfo
    {
        union
        {
            struct
            {
                SoftwareType type;
                ptr size;
                uint8 newbytes[2];
                uint8 oldbytes[2];
            } software;
            struct
            {
                HardwareSlot slot;
                HardwareType type;
                HardwareSize size;
            } hardware;
            struct
            {
                MemoryType type;
                ptr size;
            } memory;
        };
    };

    /**
    \brief Structure describing a breakpoint.
    */
    struct BreakpointInfo
    {
        ptr address;
        bool enabled;
        bool singleshoot;
        BreakpointType type;
        BreakpointInternalInfo internal;
    };

    /**
    \brief Structure for memory breakpoint management.
    */
    struct MemoryBreakpointData
    {
        uint32 Refcount;
        uint32 Type;
        DWORD OldProtect;
        DWORD NewProtect;
    };
};

#endif //DEBUGGER_BREAKPOINT_H