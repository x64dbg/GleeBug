#include "Debugger.Process.h"

namespace GleeBug
{
    bool Process::SetBreakpoint(ptr address, bool singleshoot, SoftwareType type)
    {
        //check the address
        if(!MemIsValidPtr(address) ||
                breakpoints.find({ BreakpointType::Software, address }) != breakpoints.end())
            return false;

        //setup the breakpoint information struct
        BreakpointInfo info = {};
        info.address = address;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Software;

        //determine breakpoint byte and size from the type
        switch(type)
        {
        case SoftwareType::ShortInt3:
            info.internal.software.newbytes[0] = 0xCC;
            info.internal.software.size = 1;
            break;

        default:
            return false;
        }

        //read/write the breakpoint
        if(!MemReadUnsafe(address, info.internal.software.oldbytes, info.internal.software.size))
            return false;

        if(!MemWriteUnsafe(address, info.internal.software.newbytes, info.internal.software.size))
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
        if(breakpointCallbacks.find({ BreakpointType::Software, address }) != breakpointCallbacks.end())
            return false;
        //set the breakpoint
        if(!SetBreakpoint(address, singleshoot, type))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Software, address }, cbBreakpoint });
        return true;
    }

    bool Process::DeleteBreakpoint(ptr address)
    {
        //find the breakpoint
        auto found = breakpoints.find({ BreakpointType::Software, address });
        if(found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //restore the breakpoint bytes
        if(!MemWriteUnsafe(address, info.internal.software.oldbytes, info.internal.software.size))
            return false;
        FlushInstructionCache(hProcess, nullptr, 0);

        recentlyDeletedSwbp.insert(address);

        //remove the breakpoint from the maps
        softwareBreakpointReferences.erase(info.address);
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Software, address });
        return true;
    }

    bool Process::GetFreeHardwareBreakpointSlot(HardwareSlot & slot) const
    {
        //find a free hardware breakpoint slot
        for(int i = 0; i < HWBP_COUNT; i++)
        {
            if(!hardwareBreakpoints[i].internal.hardware.enabled)
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
        if(!MemIsValidPtr(address) ||
                breakpoints.find({ BreakpointType::Hardware, address }) != breakpoints.end())
            return false;

        //attempt to set the hardware breakpoint in every thread
        bool success = true;
        for(auto & thread : threads)
        {
            if(!thread.second->SetHardwareBreakpoint(address, slot, type, size))
            {
                success = false;
                break;
            }
        }

        //if setting failed, unset all
        if(!success)
        {
            for(auto & thread : threads)
                thread.second->DeleteHardwareBreakpoint(slot);
            return false;
        }

        //setup the breakpoint information struct
        BreakpointInfo info = {};
        info.address = address;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Hardware;
        info.internal.hardware.slot = slot;
        info.internal.hardware.type = type;
        info.internal.hardware.size = size;
        info.internal.hardware.enabled = true;

        //insert in the breakpoint map
        breakpoints.insert({ { info.type, info.address }, info });

        //insert in the hardware breakpoint cache
        hardwareBreakpoints[int(slot)] = info;

        return true;
    }

    bool Process::SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type, HardwareSize size, bool singleshoot)
    {
        //check if a callback on this address was already found
        if(breakpointCallbacks.find({ BreakpointType::Hardware, address }) != breakpointCallbacks.end())
            return false;
        //set the hardware breakpoint
        if(!SetHardwareBreakpoint(address, slot, type, size, singleshoot))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Hardware, address }, cbBreakpoint });
        return true;
    }

    bool Process::DeleteHardwareBreakpoint(ptr address)
    {
        //find the hardware breakpoint
        auto found = breakpoints.find({ BreakpointType::Hardware, address });
        if(found == breakpoints.end())
            return false;
        const auto & info = found->second;

        //delete the hardware breakpoint from the internal buffer
        hardwareBreakpoints[int(info.internal.hardware.slot)].internal.hardware.enabled = false;

        //delete the hardware breakpoint from the registers
        bool success = true;
        for(auto & thread : threads)
        {
            if(!thread.second->DeleteHardwareBreakpoint(info.internal.hardware.slot))
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
    #define PAGE_WRITECOPY         0x08

    #define PAGE_EXECUTE           0x10
    #define PAGE_EXECUTE_READ      0x20
    #define PAGE_EXECUTE_READWRITE 0x40
    #define PAGE_EXECUTE_WRITECOPY 0x80

    #define PAGE_GUARD            0x100 <- not supported with PAGE_NOACCESS
    #define PAGE_NOCACHE          0x200 <- not supported with PAGE_GUARD or PAGE_WRITECOMBINE
    #define PAGE_WRITECOMBINE     0x400 <- not supported with PAGE_GUARD or PAGE_NOCACHE
    */

    static DWORD RemoveExecuteAccess(DWORD dwAccess)
    {
        //These settings can trigger access violation.
        DWORD dwBase = dwAccess & 0xFF;
        DWORD dwHigh = dwAccess & 0xFFFFFF00;
        switch(dwBase)
        {
        case PAGE_EXECUTE:
            return dwHigh | PAGE_READONLY;
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return dwHigh | (dwBase >> 4); //This removes execute in deed; https://msdn.microsoft.com/en-us/library/windows/desktop/aa366786(v=vs.85).aspx - 0x1337 tricks
        default:
            return dwAccess;
        }
    }

    static DWORD RemoveWriteAccess(DWORD dwAccess)
    {
        //Removes write permissions and write-on-copy to trigger access violation.
        DWORD dwBase = dwAccess & 0xFF;
        switch(dwBase)
        {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
            return (dwAccess & 0xFFFFFF00) | PAGE_READONLY;
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return (dwAccess & 0xFFFFFF00) | PAGE_EXECUTE_READ;
        default:
            return dwAccess;
        }
    }

    // Derive the page-wide protection from per-type reference counts. Byte-disjoint
    // breakpoint ranges may share a page, including several ranges of the same type.
    static DWORD MemoryBreakpointProtection(const MemoryBreakpointData & data, bool permanentDep)
    {
        if(data.Refcount == 0)
            return data.OldProtect;

        const bool needsGuard = data.AccessRefs != 0 || data.ReadRefs != 0 || (data.ExecuteRefs != 0 && !permanentDep);
        if(needsGuard)
        {
            // PAGE_GUARD cannot be combined with PAGE_NOACCESS, PAGE_NOCACHE, or PAGE_WRITECOMBINE.
            if((data.OldProtect & 0xFF) == PAGE_NOACCESS)
                return (data.OldProtect & ~0x7FF) | PAGE_NOACCESS;
            return (data.OldProtect & ~0x700) | PAGE_GUARD;
        }

        DWORD protect = data.OldProtect;
        if(data.ExecuteRefs != 0)
            protect = RemoveExecuteAccess(protect);
        if(data.WriteRefs != 0)
            protect = RemoveWriteAccess(protect);
        return protect;
    }

    static void RefreshMemoryBreakpointData(MemoryBreakpointData & data, bool permanentDep)
    {
        data.Refcount = data.AccessRefs + data.ReadRefs + data.WriteRefs + data.ExecuteRefs;
        data.Type = 0;
        if(data.AccessRefs != 0)
            data.Type |= uint32(MemoryType::Access);
        if(data.ReadRefs != 0)
            data.Type |= uint32(MemoryType::Read);
        if(data.WriteRefs != 0)
            data.Type |= uint32(MemoryType::Write);
        if(data.ExecuteRefs != 0)
            data.Type |= uint32(MemoryType::Execute);
        data.NewProtect = MemoryBreakpointProtection(data, permanentDep);
    }

    static bool AddMemoryBreakpointReference(MemoryBreakpointData & data, MemoryType type, bool permanentDep)
    {
        uint32* refs = nullptr;
        switch(type)
        {
        case MemoryType::Access:
            refs = &data.AccessRefs;
            break;
        case MemoryType::Read:
            refs = &data.ReadRefs;
            break;
        case MemoryType::Write:
            refs = &data.WriteRefs;
            break;
        case MemoryType::Execute:
            refs = &data.ExecuteRefs;
            break;
        }
        if(refs == nullptr || *refs == ~uint32(0))
            return false;
        ++*refs;
        RefreshMemoryBreakpointData(data, permanentDep);
        return true;
    }

    static bool RemoveMemoryBreakpointReference(MemoryBreakpointData & data, MemoryType type, bool permanentDep)
    {
        uint32* refs = nullptr;
        switch(type)
        {
        case MemoryType::Access:
            refs = &data.AccessRefs;
            break;
        case MemoryType::Read:
            refs = &data.ReadRefs;
            break;
        case MemoryType::Write:
            refs = &data.WriteRefs;
            break;
        case MemoryType::Execute:
            refs = &data.ExecuteRefs;
            break;
        }
        if(refs == nullptr || *refs == 0)
            return false;
        --*refs;
        RefreshMemoryBreakpointData(data, permanentDep);
        return true;
    }

    struct MemoryBreakpointPageAction
    {
        ptr page = 0;
        ptr allocationBase = 0;
        DWORD currentProtect = 0;
        MemoryBreakpointData data;
    };

    static DWORD TargetProtection(const MemoryBreakpointPageAction & action)
    {
        return action.data.Refcount == 0 ? action.data.OldProtect : action.data.NewProtect;
    }

    // Apply one VirtualProtectEx per maximal run with the same target protection.
    // AllocationBase is part of the key because VirtualProtectEx cannot span
    // independently reserved allocations even when their pages are consecutive.
    static bool ApplyProtectionRuns(Process & process, const std::vector<MemoryBreakpointPageAction> & actions, size_t count, bool rollback, size_t* appliedCount = nullptr)
    {
        size_t index = 0;
        while(index < count)
        {
            const auto base = actions[index].page;
            const auto allocationBase = actions[index].allocationBase;
            const DWORD protect = rollback ? actions[index].currentProtect : TargetProtection(actions[index]);
            size_t runEnd = index + 1;
            while(runEnd < count &&
                    actions[runEnd].page == actions[runEnd - 1].page + PAGE_SIZE &&
                    actions[runEnd].allocationBase == allocationBase &&
                    (rollback ? actions[runEnd].currentProtect : TargetProtection(actions[runEnd])) == protect)
            {
                ++runEnd;
            }

            const ptr byteSize = actions[runEnd - 1].page - base + PAGE_SIZE;
            if(!process.MemProtect(base, byteSize, protect))
            {
                if(appliedCount != nullptr)
                    *appliedCount = index;
                return false;
            }
            index = runEnd;
        }

        if(appliedCount != nullptr)
            *appliedCount = count;
        return true;
    }

    bool Process::SetMemoryBreakpoint(ptr address, ptr size, MemoryType type, bool singleshoot)
    {
        std::lock_guard<std::recursive_mutex> lock(memoryBreakpointMutex);
        DPRINTF();

        // Basic checks, including the range-end overflow that would otherwise wrap
        // page enumeration back to address zero.
        if(size == 0 || address > ~ptr(0) - (size - 1) || !MemIsValidPtr(address))
            return false;

        // Memory breakpoint byte ranges cannot intersect, but disjoint ranges are
        // allowed to share their first or last page.
        const ptr endAddress = address + size - 1;
        const auto range = Range(address, endAddress);
        if(memoryBreakpointRanges.find(range) != memoryBreakpointRanges.end())
            return false;

        // Stage all per-page bookkeeping before changing any protection. A single
        // VirtualQueryEx result is reused for every page in that memory region.
        const ptr alignedAddress = PAGE_ALIGN(address);
        const ptr alignedEnd = PAGE_ALIGN(endAddress);
        std::vector<MemoryBreakpointPageAction> actions;
        actions.reserve(size_t((alignedEnd - alignedAddress) / PAGE_SIZE + 1));

        MEMORY_BASIC_INFORMATION mbi = {};
        ptr regionEnd = 0;
        for(ptr page = alignedAddress;; page += PAGE_SIZE)
        {
            if(page >= regionEnd)
            {
                if(!VirtualQueryEx(hProcess, LPCVOID(page), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
                    return false;
                const ptr regionBase = ptr(mbi.BaseAddress);
                if(mbi.RegionSize > ~ptr(0) - regionBase)
                    regionEnd = ~ptr(0);
                else
                    regionEnd = regionBase + ptr(mbi.RegionSize);
                if(regionEnd <= page)
                    return false;
            }

            MemoryBreakpointPageAction action;
            action.page = page;
            action.allocationBase = ptr(mbi.AllocationBase);
            action.currentProtect = mbi.Protect;

            const auto found = memoryBreakpointPages.find(page);
            if(found != memoryBreakpointPages.end())
                action.data = found->second;
            else
                action.data.OldProtect = mbi.Protect;

            if(!AddMemoryBreakpointReference(action.data, type, permanentDep))
                return false;
            actions.push_back(action);

            if(page == alignedEnd)
                break;
        }

        // Change protections in coalesced runs. If a later run fails, restore every
        // earlier run to the actual protection observed while staging.
        size_t appliedCount = 0;
        if(!ApplyProtectionRuns(*this, actions, actions.size(), false, &appliedCount))
        {
            ApplyProtectionRuns(*this, actions, appliedCount, true);
            return false;
        }

        // Publish page metadata only after the complete protection transaction has
        // succeeded. Exception dispatch holds the same recursive mutex and therefore
        // cannot observe the debuggee and metadata in different transaction states.
        for(const auto & action : actions)
            memoryBreakpointPages[action.page] = action.data;

        // Set up and publish the byte-range breakpoint information.
        BreakpointInfo info = {};
        info.address = address;
        info.singleshoot = singleshoot;
        info.type = BreakpointType::Memory;
        info.internal.memory.type = type;
        info.internal.memory.size = size;
        breakpoints.insert({ { info.type, info.address }, info });
        memoryBreakpointRanges.insert(range);

        dprintf("SetMemoryBreakpoint(%p, %p, %d, %d): %zu pages\n", address, size, type, singleshoot, actions.size());
        return true;
    }

    bool Process::SetMemoryBreakpoint(ptr address, ptr size, const BreakpointCallback & cbBreakpoint, MemoryType type, bool singleshoot)
    {
        std::lock_guard<std::recursive_mutex> lock(memoryBreakpointMutex);

        //check if a callback on this address was already found
        if(breakpointCallbacks.find({ BreakpointType::Memory, address }) != breakpointCallbacks.end())
            return false;
        //set the memory breakpoint
        if(!SetMemoryBreakpoint(address, size, type, singleshoot))
            return false;
        //insert the callback
        breakpointCallbacks.insert({ { BreakpointType::Memory, address }, cbBreakpoint });
        return true;
    }

    bool Process::DeleteMemoryBreakpoint(ptr address)
    {
        std::lock_guard<std::recursive_mutex> lock(memoryBreakpointMutex);

        // Find the byte range containing address, then find its breakpoint record.
        const auto range = memoryBreakpointRanges.find(Range(address, address));
        if(range == memoryBreakpointRanges.end())
            return false;

        const auto found = breakpoints.find({ BreakpointType::Memory, range->first });
        if(found == breakpoints.end())
            return false;
        const BreakpointInfo info = found->second;

        // Stage decremented per-type references and target protections without
        // mutating the live page map. Region queries provide both the current
        // rollback protection and allocation boundaries for safe coalescing.
        const ptr endAddress = info.address + info.internal.memory.size - 1;
        const ptr alignedAddress = PAGE_ALIGN(info.address);
        const ptr alignedEnd = PAGE_ALIGN(endAddress);
        std::vector<MemoryBreakpointPageAction> actions;
        actions.reserve(size_t((alignedEnd - alignedAddress) / PAGE_SIZE + 1));

        MEMORY_BASIC_INFORMATION mbi = {};
        ptr regionEnd = 0;
        for(ptr page = alignedAddress;; page += PAGE_SIZE)
        {
            if(page >= regionEnd)
            {
                if(!VirtualQueryEx(hProcess, LPCVOID(page), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
                    return false;
                const ptr regionBase = ptr(mbi.BaseAddress);
                if(mbi.RegionSize > ~ptr(0) - regionBase)
                    regionEnd = ~ptr(0);
                else
                    regionEnd = regionBase + ptr(mbi.RegionSize);
                if(regionEnd <= page)
                    return false;
            }

            const auto foundData = memoryBreakpointPages.find(page);
            if(foundData == memoryBreakpointPages.end())
                return false;

            MemoryBreakpointPageAction action;
            action.page = page;
            action.allocationBase = ptr(mbi.AllocationBase);
            action.currentProtect = mbi.Protect;
            action.data = foundData->second;
            if(!RemoveMemoryBreakpointReference(action.data, info.internal.memory.type, permanentDep))
                return false;
            actions.push_back(action);

            if(page == alignedEnd)
                break;
        }

        // Keep the live metadata unchanged unless every protection run succeeds.
        // Roll back successful runs when a later run fails.
        size_t appliedCount = 0;
        if(!ApplyProtectionRuns(*this, actions, actions.size(), false, &appliedCount))
        {
            ApplyProtectionRuns(*this, actions, appliedCount, true);
            return false;
        }

        // Commit updated shared-page metadata and release pages whose final
        // breakpoint reference was removed.
        for(const auto & action : actions)
        {
            if(action.data.Refcount == 0)
                memoryBreakpointPages.erase(action.page);
            else
                memoryBreakpointPages[action.page] = action.data;
        }

        // Delete the byte-range breakpoint only after its page transaction commits.
        breakpoints.erase(found);
        breakpointCallbacks.erase({ BreakpointType::Memory, info.address });
        memoryBreakpointRanges.erase(range);
        return true;
    }

    bool Process::DeleteGenericBreakpoint(const BreakpointInfo & info)
    {
        switch(info.type)
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