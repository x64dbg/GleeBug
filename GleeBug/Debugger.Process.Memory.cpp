#include "Debugger.Process.h"

namespace GleeBug
{
    bool Process::MemReadUnsafe(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        //TODO: change page protection if reading failed
        ptr read;
        if(!bytesRead)
            bytesRead = &read;
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, (SIZE_T*)bytesRead);
    }

    bool Process::MemReadSafe(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        std::lock_guard<std::recursive_mutex> lock(breakpointMutex);

        if(!MemReadUnsafe(address, buffer, size, bytesRead))
            return false;

        //choose the filter method that has the lowest cost
        auto start = address;
        auto end = start + size;
        if(size > breakpoints.size())
        {
            for(const auto & breakpoint : breakpoints)
            {
                if(breakpoint.first.first != BreakpointType::Software)
                    continue;
                const auto & info = breakpoint.second;
                auto curAddress = info.address;
                for(ptr j = 0; j < info.internal.software.size; j++)
                {
                    if(curAddress + j >= start && curAddress + j < end)
                        ((uint8*)buffer)[curAddress + j - start] = info.internal.software.oldbytes[j];
                }
            }
        }
        else
        {
            for(ptr i = start; i < end; i++)
            {
                auto found = softwareBreakpointReferences.find(i);
                if(found == softwareBreakpointReferences.end())
                    continue;
                const auto & info = found->second->second;
                auto curAddress = info.address;
                for(ptr j = 0; j < info.internal.software.size && i < end; j++, i++)
                {
                    if(curAddress + j >= start && curAddress + j < end)
                        ((uint8*)buffer)[curAddress + j - start] = info.internal.software.oldbytes[j];
                }
                i += info.internal.software.size - 1;
            }
        }

        return true;
    }

    bool Process::MemWriteUnsafe(ptr address, const void* buffer, ptr size, ptr* bytesWritten)
    {
        ptr written;
        if(!bytesWritten)
            bytesWritten = &written;
        return !!WriteProcessMemory(this->hProcess, reinterpret_cast<void*>(address), buffer, size, (SIZE_T*)bytesWritten);
    }

    bool Process::MemWriteSafe(ptr address, const void* buffer, ptr size, ptr* bytesWritten)
    {
        std::lock_guard<std::recursive_mutex> lock(breakpointMutex);

        if(size == 0)
        {
            if(bytesWritten)
                *bytesWritten = 0;
            return true;
        }

        std::vector<uint8> copy((const uint8*)buffer, (const uint8*)buffer + size);

        auto start = address;
        auto end = start + size;

        //find overlapping software breakpoints and preserve their 0xCC bytes in the copy (so write doesn't remove breakpoints)
        //as well as track what oldbytes values need updating after successful write
        std::vector<std::tuple<uint8*, uint8, ptr>> pendingUpdates; //tuple: (pointer to oldbyte, new value, offset from start for partial write handling)

        for(auto & breakpoint : breakpoints)
        {
            if(breakpoint.first.first != BreakpointType::Software)
                continue;
            auto & info = breakpoint.second;
            auto curAddress = info.address;
            for(ptr j = 0; j < info.internal.software.size; j++)
            {
                if(curAddress + j >= start && curAddress + j < end)
                {
                    auto offset = curAddress + j - start;
                    pendingUpdates.emplace_back(&info.internal.software.oldbytes[j], copy[offset], offset);
                    copy[offset] = info.internal.software.newbytes[j];
                }
            }
        }

        //write to memory (breakpoint bytes are preserved in the copy)
        ptr written = 0;
        if(!MemWriteUnsafe(address, copy.data(), size, &written))
        {
            if(bytesWritten)
                *bytesWritten = written;
            return false;
        }

        //apply oldbytes updates only for bytes that were actually written
        for(const auto & update : pendingUpdates)
        {
            if(std::get<2>(update) < written)
                *std::get<0>(update) = std::get<1>(update);
        }

        if(bytesWritten)
            *bytesWritten = written;
        return true;
    }

    bool Process::MemIsValidPtr(ptr address) const
    {
        uint8 byte;
        return MemReadUnsafe(address, &byte, sizeof(byte));
    }

    bool Process::MemProtect(ptr address, ptr size, DWORD newProtect, DWORD* oldProtect)
    {
        DWORD dwOldProtect;
        auto vps = VirtualProtectEx(hProcess, LPVOID(address), size, newProtect, &dwOldProtect);
        if(!vps)
            return false;
        if(oldProtect)
            *oldProtect = dwOldProtect;
        return true;
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if(!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern);
        return found == -1 ? 0 : found + data;
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const uint8* pattern, size_t patternsize, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if(!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern, patternsize);
        return found == -1 ? 0 : found + data;
    }

    bool Process::MemWritePattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe)
    {
        std::vector<uint8> buffer(datasize);
        if(!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return false;
        Pattern::Write(buffer.data(), datasize, pattern);
        return MemWrite(data, buffer.data(), datasize, nullptr, safe);
    }

    bool Process::MemSearchAndReplace(ptr data, size_t datasize, const Pattern::WildcardPattern & searchpattern, const Pattern::WildcardPattern & replacepattern, bool safe)
    {
        std::vector<uint8> buffer(datasize);
        if(!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return false;
        if(!Pattern::SearchAndReplace(buffer.data(), datasize, searchpattern, replacepattern))
            return false;
        return MemWrite(data, buffer.data(), datasize, nullptr, safe);
    }
};