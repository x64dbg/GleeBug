#ifndef _DEBUGGER_BREAKPOINT_H
#define _DEBUGGER_BREAKPOINT_H

#include "Debugger.Global.h"

namespace GleeBug
{
    enum class BreakpointType
    {
        Software,
        Hardware,
        Memory
    };

    enum class SoftwareBreakpointType
    {
        ShortInt3
    };

    enum class HardwareBreakpointType
    {
        Access,
        Write,
        Execute
    };

    enum class HardwareBreakpointSize
    {
        SizeByte,
        SizeWord,
        SizeDword,
#ifdef _WIN64
        SizeQword
#endif //_WIN64
    };

    enum class MemoryBreakpointType
    {
        Acess,
        Write,
        Execute
    };

    struct BreakpointInternalInfo
    {
        union
        {
            struct
            {
                SoftwareBreakpointType type;
                ptr size;
                uint8 newbytes[2];
                uint8 oldbytes[2];
            } software;
            struct
            {
                HardwareBreakpointType type;
                HardwareBreakpointSize size;
            } hardware;
            struct
            {
                MemoryBreakpointType type;
                ptr size;
            } memory;
        };
    };

    struct BreakpointInfo
    {
        ptr address;
        bool enabled;
        bool singleshoot;
        BreakpointType type;
        BreakpointInternalInfo internal;
    };
};

#endif //_DEBUGGER_BREAKPOINT_H