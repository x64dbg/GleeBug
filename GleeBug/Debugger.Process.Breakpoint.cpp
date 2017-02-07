#include "Debugger.Process.h"

namespace GleeBug
{
    bool Process::SetBreakpoint(ptr address, bool singleshoot, SoftwareType type)
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
        case SoftwareType::ShortInt3:
            info.internal.software.newbytes[0] = 0xCC;
            info.internal.software.size = 1;
            break;

        default:
            return false;
        }

        //read/write the breakpoint
        if (!MemReadUnsafe(address, info.internal.software.oldbytes, info.internal.software.size))
            return false;

        if (!MemWriteUnsafe(address, info.internal.software.newbytes, info.internal.software.size))
            return false;
        FlushInstructionCache(hProcess, nullptr, 0);

        //insert in the breakpoint map
        auto itr = breakpoints.insert({ { info.type, info.address }, info });
        softwareBreakpointReferences[info.address] = itr.first;

        return true;
    }

    bool Process::SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshoot, SoftwareType type)
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

    bool Process::DeleteBreakpoint(ptr address)
    {
        //find the breakpoint
        auto found = breakpoints.find({ BreakpointType::Software, address });
        if (found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //restore the breakpoint bytes if the breakpoint is enabled
        if (info.enabled)
        {
            if (!MemWriteUnsafe(address, info.internal.software.oldbytes, info.internal.software.size))
                return false;
            FlushInstructionCache(hProcess, nullptr, 0);
        }

        //remove the breakpoint from the maps
        softwareBreakpointReferences.erase(info.address);
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Software, address });
        return true;
    }

    bool Process::GetFreeHardwareBreakpointSlot(HardwareSlot & slot) const
    {
        //find a free hardware breakpoint slot
        for (int i = 0; i < HWBP_COUNT; i++)
        {
            if (!hardwareBreakpoints[i].enabled)
            {
                slot = HardwareSlot(i);
                return true;
            }
        }
        return false;
    }

    bool Process::SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type, HardwareSize size, bool singleshoot)
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

    bool Process::SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type, HardwareSize size, bool singleshoot)
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

    bool Process::DeleteHardwareBreakpoint(ptr address)
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

#define PAGE_SHIFT              (12)
#define PAGE_ALIGN(Va)          ((ULONG_PTR)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTES_TO_PAGES(Size)    (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define ROUND_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

    /*
    #define PAGE_NOACCESS          0x01
    #define PAGE_READONLY          0x02
    #define PAGE_READWRITE         0x04
    #define PAGE_WRITECOPY         0x08 <- not supported

    #define PAGE_EXECUTE           0x10
    #define PAGE_EXECUTE_READ      0x20
    #define PAGE_EXECUTE_READWRITE 0x40
    #define PAGE_EXECUTE_WRITECOPY 0x80 <- not supported

    #define PAGE_GUARD            0x100 <- not supported with PAGE_NOACCESS
    #define PAGE_NOCACHE          0x200 <- not supported with PAGE_GUARD or PAGE_WRITECOMBINE
    #define PAGE_WRITECOMBINE     0x400 <- not supported with PAGE_GUARD or PAGE_NOCACHE
    */

    static DWORD RemoveExecuteAccess(DWORD dwAccess)
    {
        DWORD dwBase = dwAccess & 0xFF;
        DWORD dwHigh = dwAccess & 0xFFFFFF00;
        switch (dwBase)
        {
        case PAGE_EXECUTE:
            return dwHigh | PAGE_READONLY;
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return dwHigh | (dwBase >> 4);
        default:
            return dwAccess;
        }
    }

    static DWORD RemoveWriteAccess(DWORD dwAccess)
    {
        DWORD dwBase = dwAccess & 0xFF;
        switch (dwBase)
        {
        case PAGE_READWRITE:
        case PAGE_EXECUTE_READWRITE:
            return (dwAccess & 0xFFFFFF00) | (dwBase >> 1);
        default:
            return dwAccess;
        }
    }

    bool Process::SetNewPageProtection(ptr page, MemoryBreakpointData & data, MemoryType type)
    {
        //TODO: handle PAGE_NOACCESS and such correctly (since it cannot be combined with PAGE_GUARD)

        auto found = memoryBreakpointPages.find(page);
        if (found == memoryBreakpointPages.end())
        {
            data.Refcount = 1;
            switch (type)
            {
            case MemoryType::Access:
            case MemoryType::Read:
                data.NewProtect = data.OldProtect | PAGE_GUARD;
                break;
            case MemoryType::Write:
                data.NewProtect = RemoveWriteAccess(data.OldProtect);
                break;
            case MemoryType::Execute:
                data.NewProtect = permanentDep ? RemoveExecuteAccess(data.OldProtect) : data.OldProtect | PAGE_GUARD;
                break;
            }
        }
        else
        {
            auto & oldData = found->second;
            data.Type = oldData.Type | uint32(type);
            data.OldProtect = oldData.OldProtect;
            data.Refcount = oldData.Refcount + 1;
            if (data.Type & uint32(MemoryType::Access) || data.Type & uint32(MemoryType::Read)) //Access/Read always becomes PAGE_GUARD
                data.NewProtect = data.OldProtect | PAGE_GUARD;
            else if (data.Type & (uint32(MemoryType::Write) | uint32(MemoryType::Execute))) //Write + Execute becomes either PAGE_GUARD or both write and execute flags removed
                data.NewProtect = permanentDep ? RemoveExecuteAccess(RemoveWriteAccess(data.OldProtect)) : data.OldProtect | PAGE_GUARD;
        }

        return MemProtect(page, PAGE_SIZE, data.NewProtect);
    }

    bool Process::SetMemoryBreakpoint(ptr address, ptr size, MemoryType type, bool singleshoot)
    {
        //TODO: error reporting

        //basic checks
        if (!MemIsValidPtr(address) || !size)
            return false;

        //check if the range is unused for any previous memory breakpoints
        auto range = Range(address, address + size - 1);
        if (memoryBreakpointRanges.find(range) != memoryBreakpointRanges.end())
            return false;

        //change page protections
        bool success = true;
        struct TempMemoryBreakpointData
        {
            ptr addr;
            DWORD OldProtect;
            MemoryBreakpointData data;
        };
        std::vector<TempMemoryBreakpointData> breakpointData;
        {
            breakpointData.reserve(size / PAGE_SIZE);
            TempMemoryBreakpointData tempData;
            MemoryBreakpointData data;
            data.Type = uint32(type);
            auto alignedAddress = PAGE_ALIGN(address);
            for (auto page = alignedAddress; page < alignedAddress + BYTES_TO_PAGES(size); page += PAGE_SIZE)
            {
                MEMORY_BASIC_INFORMATION mbi;
                if (!VirtualQueryEx(hProcess, LPCVOID(page), &mbi, sizeof(mbi)))
                {
                    success = false;
                    break;
                }
                data.OldProtect = mbi.Protect;
                if (!SetNewPageProtection(page, data, type))
                {
                    success = false;
                    break;
                }
                tempData.addr = page;
                tempData.OldProtect = mbi.Protect;
                tempData.data = data;
                breakpointData.push_back(tempData);
            }
        }

        //if changing the page protections failed, attempt to revert all protection changes
        if (!success)
        {
            for (const auto & page : breakpointData)
                MemProtect(page.addr, PAGE_SIZE, page.OldProtect);
            return false;
        }

        //set the page data
        for (const auto & page : breakpointData)
            memoryBreakpointPages[page.addr] = page.data;

        //setup the breakpoint information struct
        BreakpointInfo info = {};
        info.address = address;
        info.enabled = true;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Memory;
        info.internal.memory.type = type;
        info.internal.memory.size = size;

        //insert in the breakpoint map
        breakpoints.insert({ { info.type, info.address }, info });
        memoryBreakpointRanges.insert(range);

        return true;
    }

    bool Process::SetMemoryBreakpoint(ptr address, ptr size, const BreakpointCallback & cbBreakpoint, MemoryType type, bool singleshoot)
    {
        //check if a callback on this address was already found
        if (breakpointCallbacks.find({ BreakpointType::Memory, address }) != breakpointCallbacks.end())
            return false;
        //set the memory breakpoint
        if (!SetMemoryBreakpoint(address, size, type, singleshoot))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Memory, address }, cbBreakpoint });
        return true;
    }

    bool Process::DeleteMemoryBreakpoint(ptr address)
    {
        //find the memory breakpoint range
        auto range = memoryBreakpointRanges.find(Range(address, address));
        if(range == memoryBreakpointRanges.end())
            return false;

        //find the memory breakpoint
        auto found = breakpoints.find({ BreakpointType::Memory, range->first });
        if (found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //delete the memory breakpoint from the pages
        bool success = true;
        auto alignedAddress = PAGE_ALIGN(info.address);
        for (auto page = alignedAddress; page < alignedAddress + BYTES_TO_PAGES(info.internal.memory.size); page += PAGE_SIZE)
        {
            auto foundData = memoryBreakpointPages.find(page);
            if (foundData == memoryBreakpointPages.end())
                continue; //TODO: error reporting
            auto & data = foundData->second;
            DWORD Protect;
            data.Refcount--;
            if (data.Refcount)
            {
                //TODO: properly determine the new protection flag
                if (data.Type & ~uint32(info.internal.memory.type))
                    data.NewProtect = data.OldProtect | PAGE_GUARD;
                Protect = data.NewProtect;
            }
            else
                Protect = data.OldProtect;
            if (!MemProtect(page, PAGE_SIZE, Protect))
                success = false;
            if (!data.Refcount)
                memoryBreakpointPages.erase(foundData);
        }

        //delete the breakpoint from the maps
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Hardware, address });
        memoryBreakpointRanges.erase(Range(address, address));
        return success;
    }

    bool Process::DeleteGenericBreakpoint(const BreakpointInfo & info)
    {
        switch (info.type)
        {
        case BreakpointType::Software:
            return DeleteBreakpoint(info.address);
        case BreakpointType::Hardware:
            return DeleteHardwareBreakpoint(info.address);
        case BreakpointType::Memory:
            return DeleteMemoryBreakpoint(info.address);
        default:
            return false;
        }
    }
};