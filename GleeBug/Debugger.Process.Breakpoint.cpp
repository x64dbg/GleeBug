#include "Debugger.Process.h"

namespace GleeBug
{
    bool ProcessInfo::SetBreakpoint(ptr address, bool singleshoot, SoftwareBreakpointType type)
    {
        //check the address
        if (!MemIsValidPtr(address) ||
            breakpoints.find({ BreakpointType::Software, address }) != breakpoints.end())
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
        auto itr = breakpoints.insert({ { info.type, info.address }, info });
        softwareBreakpointReferences[info.address] = itr.first;

        return true;
    }

    bool ProcessInfo::SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshoot, SoftwareBreakpointType type)
    {
        //check if a callback on this address was already found
        if (breakpointCallbacks.find({ BreakpointType::Software, address }) != breakpointCallbacks.end())
            return false;
        //set the breakpoint
        if (!SetBreakpoint(address, singleshoot, type))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Software, address }, cbBreakpoint });
        return true;
    }

    bool ProcessInfo::DeleteBreakpoint(ptr address)
    {
        //find the breakpoint
        auto found = breakpoints.find({ BreakpointType::Software, address });
        if (found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //restore the breakpoint bytes if the breakpoint is enabled
        if (info.enabled)
        {
            if (!MemWrite(address, info.internal.software.oldbytes, info.internal.software.size))
                return false;
            FlushInstructionCache(hProcess, nullptr, 0);
        }

        //remove the breakpoint from the maps
        softwareBreakpointReferences.erase(info.address);
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Software, address });
        return true;
    }

    bool ProcessInfo::GetFreeHardwareBreakpointSlot(HardwareBreakpointSlot & slot) const
    {
        //find a free hardware breakpoint slot
        for (int i = 0; i < HWBP_COUNT; i++)
        {
            if (!hardwareBreakpoints[i].enabled)
            {
                slot = HardwareBreakpointSlot(i);
                return true;
            }
        }
        return false;
    }

    bool ProcessInfo::SetHardwareBreakpoint(ptr address, HardwareBreakpointSlot slot, HardwareBreakpointType type, HardwareBreakpointSize size, bool singleshoot)
    {
        //check the address
        if (!MemIsValidPtr(address) ||
            breakpoints.find({ BreakpointType::Hardware, address }) != breakpoints.end())
            return false;

        //attempt to set the hardware breakpoint in every thread
        bool success = true;
        for (auto & thread : threads)
        {
            if (!thread.second.SetHardwareBreakpoint(address, slot, type, size))
            {
                success = false;
                break;
            }
        }

        //if setting failed, unset all
        if (!success)
        {
            for (auto & thread : threads)
                thread.second.DeleteHardwareBreakpoint(slot);
            return false;
        }

        //setup the breakpoint information struct
        BreakpointInfo info = {};
        info.address = address;
        info.enabled = true;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Hardware;
        info.internal.hardware.slot = slot;
        info.internal.hardware.type = type;
        info.internal.hardware.size = size;

        //insert in the breakpoint map
        breakpoints.insert({ { info.type, info.address }, info });

        //insert in the hardware breakpoint cache
        hardwareBreakpoints[int(slot)] = info;

        return true;
    }

    bool ProcessInfo::SetHardwareBreakpoint(ptr address, HardwareBreakpointSlot slot, const BreakpointCallback & cbBreakpoint, HardwareBreakpointType type, HardwareBreakpointSize size, bool singleshoot)
    {
        //check if a callback on this address was already found
        if (breakpointCallbacks.find({ BreakpointType::Hardware, address }) != breakpointCallbacks.end())
            return false;
        //set the hardware breakpoint
        if (!SetHardwareBreakpoint(address, slot, type, size, singleshoot))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Hardware, address }, cbBreakpoint });
        return true;
    }

    bool ProcessInfo::DeleteHardwareBreakpoint(ptr address)
    {
        //find the hardware breakpoint
        auto found = breakpoints.find({ BreakpointType::Hardware, address });
        if (found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //delete the hardware breakpoint from the internal buffer
        hardwareBreakpoints[int(info.internal.hardware.slot)].enabled = false;

        //delete the hardware breakpoint from the registers
        bool success = true;
        for (auto & thread : threads)
        {
            if (!thread.second.DeleteHardwareBreakpoint(info.internal.hardware.slot))
                success = false;
        }

        //delete the breakpoint from the maps
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Hardware, address });
        return success;
    }

    bool ProcessInfo::DeleteGenericBreakpoint(const BreakpointInfo & info)
    {
        switch (info.type)
        {
        case BreakpointType::Software:
            return DeleteBreakpoint(info.address);
        case BreakpointType::Hardware:
            return DeleteHardwareBreakpoint(info.address);
        case BreakpointType::Memory:
            return false; //TODO implement this
        default:
            return false;
        }
    }
};