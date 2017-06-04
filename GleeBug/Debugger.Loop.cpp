#include "Debugger.h"

namespace GleeBug
{
    void Debugger::Start()
    {
        //initialize loop variables
        mBreakDebugger = false;
        mIsDebugging = true;
        mDetach = false;
        mDetachAndBreak = false;

        //use correct WaitForDebugEvent function
        typedef BOOL(WINAPI *MYWAITFORDEBUGEVENT)(
            _Out_ LPDEBUG_EVENT lpDebugEvent,
            _In_  DWORD         dwMilliseconds
            );
        static auto WFDEX = MYWAITFORDEBUGEVENT(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WaitForDebugEventEx"));
        static auto MyWaitForDebugEvent = WFDEX ? WFDEX : MYWAITFORDEBUGEVENT(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WaitForDebugEvent"));
        if (!MyWaitForDebugEvent)
        {
            cbInternalError("MyWaitForDebugEvent not set!");
            return;
        }

        while (!mBreakDebugger)
        {
            //wait for a debug event
            mIsRunning = true;
            if (!MyWaitForDebugEvent(&mDebugEvent, INFINITE))
                break;
            mIsRunning = false;

            //set default continue status
            mContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

            //set the current process and thread
            auto processFound = mProcesses.find(mDebugEvent.dwProcessId);
            if (processFound != mProcesses.end())
            {
                mProcess = processFound->second.get();
                auto threadFound = mProcess->threads.find(mDebugEvent.dwThreadId);
                if (threadFound != mProcess->threads.end())
                {
                    mThread = mProcess->thread = threadFound->second.get();
                    mRegisters = &mThread->registers;
                }
                else
                {
                    mThread = mProcess->thread = nullptr;
                    mRegisters = nullptr;
                }
            }
            else
            {
                mRegisters = nullptr;
                mThread = nullptr;
                if (mProcess)
                {
                    mProcess->thread = nullptr;
                    mProcess = nullptr;
                }
            }

            //read register contexts
            if(mProcess && !mProcess->RegReadContext())
                cbInternalError("Process::RegReadContext() failed!");                        

            //call the pre debug event callback
            cbPreDebugEvent(mDebugEvent);

            //dispatch the debug event (documented here: https://msdn.microsoft.com/en-us/library/windows/desktop/ms679302(v=vs.85).aspx)
            switch (mDebugEvent.dwDebugEventCode)
            {
            case CREATE_PROCESS_DEBUG_EVENT:
                createProcessEvent(mDebugEvent.u.CreateProcessInfo);
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                exitProcessEvent(mDebugEvent.u.ExitProcess);
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                createThreadEvent(mDebugEvent.u.CreateThread);
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                exitThreadEvent(mDebugEvent.u.ExitThread);
                break;
            case LOAD_DLL_DEBUG_EVENT:
                loadDllEvent(mDebugEvent.u.LoadDll);
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                unloadDllEvent(mDebugEvent.u.UnloadDll);
                break;
            case EXCEPTION_DEBUG_EVENT:
                exceptionEvent(mDebugEvent.u.Exception);
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                debugStringEvent(mDebugEvent.u.DebugString);
                break;
            case RIP_EVENT:
                ripEvent(mDebugEvent.u.RipInfo);
                break;
            default:
                unknownEvent(mDebugEvent.dwDebugEventCode);
                break;
            }

            //call the post debug event callback
            cbPostDebugEvent(mDebugEvent);

            //execute the delayed-detach
            if (mDetachAndBreak)
            {
                if (!UnsafeDetachAndBreak())
                    cbInternalError("Debugger::DetachAndBreak failed!");
                break;
            }

            //clear trap flag when set by GleeBug (to prevent an EXCEPTION_SINGLE_STEP after detach)
            if (mDetach && mThread)
            {
                if (mThread->isInternalStepping || mThread->isSingleStepping)
                    mThread->registers.TrapFlag = false;
            }

            //write register contexts
            if(mProcess && !mProcess->RegWriteContext())
                cbInternalError("Process::RegWriteContext() failed!");

            //continue the debug event
            if (!ContinueDebugEvent(mDebugEvent.dwProcessId, mDebugEvent.dwThreadId, mContinueStatus))
                break;

            if (mDetach || mDetachAndBreak)
            {
                if (!UnsafeDetach())
                    cbInternalError("Debugger::Detach failed!");
                break;
            }
        }

        //cleanup
        mProcesses.clear();
        mProcess = nullptr;
        mIsDebugging = false;
    }
};