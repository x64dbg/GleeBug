#ifndef _DEBUGGER_PROCESS_H
#define _DEBUGGER_PROCESS_H

#include "Debugger.Global.h"
#include "Debugger.Thread.h"
#include "Debugger.Dll.h"

namespace GleeBug
{
    /**
    \brief Process information structure.
    */
    class ProcessInfo
    {
    public:
        HANDLE hProcess;
        uint32 dwProcessId;
        uint32 dwMainThreadId;

        ThreadInfo* thread;
        bool systemBreakpoint;

        ThreadMap threads;
        DllMap dlls;

        /**
        \brief Constructor.
        \param dwProcessId Identifier for the process.
        \param dwMainThreadId Identifier for the main thread.
        */
        ProcessInfo(uint32 dwProcessId, HANDLE hProcess, uint32 dwMainThreadId);

        /**
        \brief Read memory from the process.
        \param address The virtual address to read from.
        \param [out] buffer Destination buffer. Cannot be null. May be filled partially on failure.
        \param size The size to read.
        \return true if it succeeds, false if it fails.
        */
        bool MemRead(ptr address, void* buffer, const size_t size) const;

        /**
        \brief Write memory to the process.
        \param address The virtual address to write to.
        \param [in] buffer Source buffer. Cannot be null.
        \param size The size to write.
        \return true if it succeeds, false if it fails.
        */
        bool MemWrite(ptr address, const void* buffer, const size_t size) const;
    };
};

#endif //_DEBUGGER_PROCESS_H