#include "Debugger.h"

namespace GleeBug
{
    void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
    {
        //initial attach housekeeping
        if(mAttachedToProcess && !mMainProcess.dwProcessId)
        {
            mMainProcess.hProcess = createProcess.hProcess;
            mMainProcess.hThread = createProcess.hThread;
            mMainProcess.dwProcessId = mDebugEvent.dwProcessId;
            mMainProcess.dwThreadId = mDebugEvent.dwThreadId;
            cbAttachBreakpoint();
        }

        //process housekeeping
        mProcesses.insert({ mDebugEvent.dwProcessId,
            std::make_unique<Process>(createProcess.hProcess,
            mDebugEvent.dwProcessId,
            mDebugEvent.dwThreadId,
            createProcess) });
        mProcess = mProcesses.find(mDebugEvent.dwProcessId)->second.get();

        //thread housekeeping (main thread is created implicitly)
        mProcess->threads.insert({ mDebugEvent.dwThreadId,
            std::make_unique<Thread>(createProcess.hThread,
            mDebugEvent.dwThreadId,
            createProcess.lpThreadLocalBase,
            createProcess.lpStartAddress) });
        mThread = mProcess->thread = mProcess->threads.find(mDebugEvent.dwThreadId)->second.get();
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