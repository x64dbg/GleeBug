#ifndef _MYDEBUGGER_H
#define _MYDEBUGGER_H

#include "../GleeBug/Debugger.h"

using namespace GleeBug;

class MyDebugger : public Debugger
{
protected:
    void myBreakpoint(const BreakpointInfo & info)
    {
        puts("myBreakpoint()");
    }

    void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess, const ProcessInfo & process) override
    {
        ptr entry = ptr(createProcess.lpStartAddress);
        printf("Process %d created with entry 0x%p\n",
            _debugEvent.dwProcessId,
            entry);
        if(_process->SetBreakpoint(entry, this, &MyDebugger::myBreakpoint))
            printf("Breakpoint set at 0x%p!\n", entry);
        else
            printf("Failed to set breakpoint at 0x%p...\b", entry);
    }

    void cbExitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess, const ProcessInfo & process) override
    {
        printf("Process %u terminated with exit code 0x%08X\n",
            _debugEvent.dwProcessId,
            exitProcess.dwExitCode);
    }

    void cbCreateThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread, const ThreadInfo & thread) override
    {
        printf("Thread %u created with entry 0x%p\n",
            _debugEvent.dwThreadId,
            createThread.lpStartAddress);
    }

    void cbExitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread, const ThreadInfo & thread) override
    {
        printf("Thread %u terminated with exit code 0x%08X\n",
            _debugEvent.dwThreadId,
            exitThread.dwExitCode);
    }

    void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll, const DllInfo & dll) override
    {
        printf("DLL loaded at 0x%p\n",
            loadDll.lpBaseOfDll);
    }

    void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll, const DllInfo & dll) override
    {
        printf("DLL 0x%p unloaded\n",
            unloadDll.lpBaseOfDll);
    }

    void cbExceptionEvent(const EXCEPTION_DEBUG_INFO & exceptionInfo) override
    {
        const char* exceptionType = exceptionInfo.dwFirstChance ? "First Chance" : "Second Chance";
        printf("%s exception with code 0x%08X at 0x%p\n",
            exceptionType,
            exceptionInfo.ExceptionRecord.ExceptionCode,
            exceptionInfo.ExceptionRecord.ExceptionAddress);
    }

    void cbDebugStringEvent(const OUTPUT_DEBUG_STRING_INFO & debugString) override
    {
        printf("Debug string at 0x%p with length %d\n",
            debugString.lpDebugStringData,
            debugString.nDebugStringLength);
    }

    void cbRipEvent(const RIP_INFO & rip) override
    {
        printf("RIP event type 0x%X, error 0x%X",
            rip.dwType,
            rip.dwError);
    }

    void boobs()
    {
        printf("(.)Y(.) 0x%p\n",
            _registers->Gip.Get());
    }

    void gax()
    {
        printf("GAX: 0x%p = 0x%p = 0x%p\n",
            _registers->Get(Registers::R::GAX),
            _registers->Gax.Get(),
            _registers->Gax());
    }

    void cbSystemBreakpoint() override
    {
        printf("System breakpoint reached, CIP: 0x%p\n",
            _registers->Gip.Get());
        _thread->StepInto(this, &MyDebugger::boobs);
    }

    void cbInternalError(const std::string & error) override
    {
        printf("Internal Error: %s\n",
            error.c_str());
    }

    void cbBreakpoint(const BreakpointInfo & info) override
    {
        printf("Breakpoint on 0x%p!\n",
            info.address);
    }
};

#endif //_MYDEBUGGER_H