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
        return false;
    }

    bool Process::MemIsValidPtr(ptr address) const
    {
        uint8 byte;
        return MemReadUnsafe(address, &byte, sizeof(byte));
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const std::vector<Pattern::Byte> & pattern, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern);
        return found == -1 ? 0 : found + data;
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const char* pattern, bool safe) const
    {
        return MemFindPattern(data, datasize, Pattern::Transform(pattern), safe);
    }

    ptr Process::MemFindPattern(ptr data, size_t datasize, const uint8* pattern, size_t patternsize, bool safe) const
    {
        std::vector<uint8> buffer(datasize);
        if (!MemRead(data, buffer.data(), datasize, nullptr, safe))
            return 0;
        auto found = Pattern::Find(buffer.data(), datasize, pattern, patternsize);
        return found == -1 ? 0 : found + data;
    }
};