#ifndef _MYDEBUGGER_H
#define _MYDEBUGGER_H

#include "../GleeBug/Debugger.h"

using namespace GleeBug;

class MyDebugger : public Debugger
{
protected:
    void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess, const ProcessInfo & process) override
    {
        printf("Process %d created with entry 0x%p\n",
            _debugEvent.dwProcessId,
            createProcess.lpStartAddress);
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
        printf("Exception with code 0x%08X at 0x%p\n",
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
        printf("%p\n", _registers->Gcx());
        gax();
        _registers->Gax.Set(123);
        gax();
        _registers->Gax = 0x1234;
        if (_registers->Gax == _registers->Gcx())
        {
            puts("test== okay!");
        }
        if (_registers->Gax != 1)
            puts("test!= okay!");
        gax();
        _registers->Gax++;
        gax();
        ++_registers->Gax;
        gax();


        printf("System breakpoint reached, CIP: 0x%p\n",
            _registers->Gip.Get());
        _thread->StepInto(BIND(this, MyDebugger::boobs));
    }

    void cbInternalError(const std::string & error) override
    {
        printf("Internal Error: %s\n",
            error.c_str());
    }
};

#endif //_MYDEBUGGER_H