#include "Debugger.Process.h"

namespace GleeBug
{
    bool ProcessInfo::SetBreakpoint(ptr address, bool singleshoot, SoftwareBreakpointType type)
    {
        //check the address
        if (!MemIsValidPtr(address) ||
            breakpoints.find({BreakpointType::Software, address}) != breakpoints.end())
            return false;

        //setup the breakpoint information struct
        BreakpointInfo info = {};
        info.address = address;
        info.enabled = true;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Software;
        
        //determine breakpoint byte and size from the type
        switch (type)
        {
        case SoftwareBreakpointType::ShortInt3:
            info.internal.software.newbytes[0] = 0xCC;
            info.internal.software.size = 1;
            break;

        default:
            return false;
        }

        //read/write the breakpoint
        if (!MemRead(address, info.internal.software.oldbytes, info.internal.software.size))
            return false;

        if (!MemWrite(address, info.internal.software.newbytes, info.internal.software.size))
            return false;
        FlushInstructionCache(hProcess, nullptr, 0);

        //insert in the breakpoint map
        breakpoints.insert({ { info.type, info.address }, info });

        return true;
    }

    bool ProcessInfo::SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshoot, SoftwareBreakpointType type)
    {
        if (!SetBreakpoint(address, singleshoot, type))
            return false;
        auto found = breakpointCallbacks.find({ BreakpointType::Software, address });
        if (found != breakpointCallbacks.end())
            return false;
        breakpointCallbacks.insert({ { BreakpointType::Software, address }, cbBreakpoint });
        return true;
    }
};