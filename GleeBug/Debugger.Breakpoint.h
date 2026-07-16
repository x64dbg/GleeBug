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
        Read = 2,
        Write = 4,
        Execute = 8
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
                bool enabled;
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
        bool singleshoot;
        BreakpointType type;
        BreakpointInternalInfo internal;
    };

    /**
    \brief Structure for memory breakpoint management.
    */
    struct MemoryBreakpointData
    {
        // Refcount and Type are cached aggregates used by existing page-handling
        // code. Per-type counts preserve multiplicity when same-type ranges share
        // one page and allow deletion to derive the exact remaining protection.
        uint32 Refcount = 0;
        uint32 Type = 0;
        uint32 AccessRefs = 0;
        uint32 ReadRefs = 0;
        uint32 WriteRefs = 0;
        uint32 ExecuteRefs = 0;
        DWORD OldProtect = 0;
        DWORD NewProtect = 0;
    };
};

#endif //DEBUGGER_BREAKPOINT_H