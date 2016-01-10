#include "Debugger.h"

namespace GleeBug
{
    void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
    {
        //process housekeeping
        mProcesses.insert({ mDebugEvent.dwProcessId,
            Process(createProcess.hProcess,
            mDebugEvent.dwProcessId,
            mDebugEvent.dwThreadId) });
        mProcess = &mProcesses.find(mDebugEvent.dwProcessId)->second;

        //thread housekeeping (main thread is created implicitly)
        mProcess->threads.insert({ mDebugEvent.dwThreadId,
            Thread(createProcess.hThread,
            mDebugEvent.dwThreadId,
            createProcess.lpThreadLocalBase,
            createProcess.lpStartAddress) });
        mThread = mProcess->thread = &mProcess->threads.find(mDebugEvent.dwThreadId)->second;
        mRegisters = &mThread->registers;

        //read thread context from main thread
        if (!mThread->RegReadContext())
            cbInternalError("Thread::RegReadContext() failed!");

        //call the debug event callback
        cbCreateProcessEvent(createProcess, *mProcess);

        //close the file handle
        CloseHandle(createProcess.hFile);
    }

    void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
    {
        //check if the terminated process is the main debuggee
        if (mDebugEvent.dwProcessId == mMainProcess.dwProcessId)
            mBreakDebugger = true;

        //call the debug event callback
        cbExitProcessEvent(exitProcess, *mProcess);

        //process housekeeping
        mProcesses.erase(mDebugEvent.dwProcessId);

        //set the current process
        mProcess = nullptr;
        mThread = nullptr;
        mRegisters = nullptr;
    }
};