#include "Debugger.Process.h"

namespace GleeBug
{
    bool Process::MemReadUnsafe(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        ptr read;
        if (!bytesRead)
            bytesRead = &read;
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, (SIZE_T*)bytesRead);
    }

    bool Process::MemReadSafe(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        if (!MemReadUnsafe(address, buffer, size, bytesRead))
            return false;

        //choose the filter method that has the lowest cost
        auto start = address;
        auto end = start + size;
        if (size > breakpoints.size())
        {
            for (const auto & breakpoint : breakpoints)
            {
                if (breakpoint.first.first != BreakpointType::Software)
                    continue;
                const auto & info = breakpoint.second;
                auto curAddress = info.address;
                for (ptr j = 0; j < info.internal.software.size; j++)
                {
                    if (curAddress + j >= start && curAddress + j < end)
                        ((uint8*)buffer)[curAddress + j - start] = info.internal.software.oldbytes[j];
                }
            }
        }
        else
        {
            for (ptr i = start; i < end; i++)
            {
                auto found = softwareBreakpointReferences.find(i);
                if (found == softwareBreakpointReferences.end())
                    continue;
                const auto & info = found->second->second;
                auto curAddress = info.address;
                for (ptr j = 0; j < info.internal.software.size && i < end; j++, i++)
                {
                    if (curAddress + j >= start && curAddress + j < end)
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
        if (!bytesWritten)
            bytesWritten = &written;
        return !!WriteProcessMemory(this->hProcess, reinterpret_cast<void*>(address), buffer, size, (SIZE_T*)bytesWritten);
    }

    bool Process::MemWriteSafe(ptr address, const void* buffer, ptr size, ptr* bytesWritten)
    {
        //TODO: correctly implement this
        return MemWrite(address, buffer, size, bytesWritten);
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
        if (!vps)
            return false;
        if (oldProtect)
            *oldProtect = dwOldProtect;
        return true;
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern);
        return found == -1 ? 0 : found + data;
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const uint8* pattern, size_t patternsize, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern, patternsize);
        return found == -1 ? 0 : found + data;
    }

    bool Process::MemWritePattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe)
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return false;
        Pattern::Write(buffer.data(), datasize, pattern);
        return MemWrite(data, buffer.data(), datasize, nullptr, safe);
    }

    bool Process::MemSearchAndReplace(ptr data, size_t datasize, const Pattern::WildcardPattern & searchpattern, const Pattern::WildcardPattern & replacepattern, bool safe)
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return false;
        if (!Pattern::SearchAndReplace(buffer.data(), datasize, searchpattern, replacepattern))
            return false;
        return MemWrite(data, buffer.data(), datasize, nullptr, safe);
    }
};