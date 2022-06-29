#include "Debugger.h"

namespace GleeBug
{
    void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
    {
        //initial attach housekeeping
        bool attachBreakpoint = false;
        if(mAttachedToProcess && !mMainProcess.dwProcessId)
        {
            mMainProcess.hProcess = createProcess.hProcess;
            mMainProcess.hThread = createProcess.hThread;
            mMainProcess.dwProcessId = mDebugEvent.dwProcessId;
            mMainProcess.dwThreadId = mDebugEvent.dwThreadId;
            attachBreakpoint = true;
        }

        //process housekeeping
        mProcesses.insert({ mDebugEvent.dwProcessId,
                            std::make_unique<Process>(createProcess.hProcess,
                                    mDebugEvent.dwProcessId,
                                    mDebugEvent.dwThreadId,
                                    createProcess)
                          });
        mProcess = mProcesses.find(mDebugEvent.dwProcessId)->second.get();

        //thread housekeeping (main thread is created implicitly)
        mProcess->threads.insert({ mDebugEvent.dwThreadId,
                                   std::make_unique<Thread>(createProcess.hThread,
                                           mDebugEvent.dwThreadId,
                                           createProcess.lpThreadLocalBase,
                                           createProcess.lpStartAddress)
                                 });
        mThread = mProcess->thread = mProcess->threads.find(mDebugEvent.dwThreadId)->second.get();

        //call the debug event callback
        cbCreateProcessEvent(createProcess, *mProcess);

        //close the file handle
        if(createProcess.hFile)
            CloseHandle(createProcess.hFile);

        //call attach breakpoint after process creation
        if(attachBreakpoint)
            cbAttachBreakpoint();
    }

    void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
    {
        //check if the terminated process is the main debuggee
        if(mDebugEvent.dwProcessId == mMainProcess.dwProcessId)
            mBreakDebugger = true;

        //call the debug event callback
        cbExitProcessEvent(exitProcess, *mProcess);

        //process housekeeping
        mProcesses.erase(mDebugEvent.dwProcessId);

        //set the current process
        mProcess = nullptr;
        mThread = nullptr;
    }
};