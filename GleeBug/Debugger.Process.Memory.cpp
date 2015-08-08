#include "Debugger.Process.h"

namespace GleeBug
{
    bool ProcessInfo::MemRead(ptr address, void* buffer, ptr size) const
    {
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, nullptr);
    }

    bool ProcessInfo::MemWrite(ptr address, const void* buffer, ptr size)
    {
        return !!WriteProcessMemory(this->hProcess, reinterpret_cast<void*>(address), buffer, size, nullptr);
    }

    bool ProcessInfo::MemIsValidPtr(ptr address) const
    {
        uint8 byte;
        return MemRead(address, &byte, sizeof(byte));
    }
};