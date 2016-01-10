#include "Debugger.Process.h"

namespace GleeBug
{
    bool Process::MemRead(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        ptr read;
        if (!bytesRead)
            bytesRead = &read;
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, (SIZE_T*)bytesRead);
    }

    bool Process::MemReadSafe(ptr address, void* buffer, ptr size, ptr* bytesRead) const
    {
        if (!MemRead(address, buffer, size, bytesRead))
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

    bool Process::MemWrite(ptr address, const void* buffer, ptr size, ptr* bytesWritten)
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
        return MemRead(address, &byte, sizeof(byte));
    }
};