#include "Debugger.h"
#include "Debugger.Thread.Registers.h"

#ifndef DBG_REPLY_LATER
#define DBG_REPLY_LATER ((NTSTATUS)0x40010001L)
#endif // DBG_REPLY_LATER

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
        typedef BOOL(WINAPI * MYWAITFORDEBUGEVENT)(
            _Out_ LPDEBUG_EVENT lpDebugEvent,
            _In_  DWORD         dwMilliseconds
        );
        static auto WFDEX = MYWAITFORDEBUGEVENT(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WaitForDebugEventEx"));
        static auto MyWaitForDebugEvent = WFDEX ? WFDEX : MYWAITFORDEBUGEVENT(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WaitForDebugEvent"));
        if(!MyWaitForDebugEvent)
        {
            cbInternalError("MyWaitForDebugEvent not set!");
            return;
        }

        DWORD ThreadBeingProcessed = 0;
        std::unordered_map<DWORD, HANDLE> SuspendedThreads;
        bool IsDbgReplyLaterSupported = false;

        // Check if DBG_REPLY_LATER is supported based on Windows version (Windows 10, version 1507 or above)
        // https://www.gaijin.at/en/infos/windows-version-numbers
        const uint32_t NtBuildNumber = *(uint32_t*)(0x7FFE0000 + 0x260);
        if(NtBuildNumber != 0 && NtBuildNumber >= 10240)
        {
            IsDbgReplyLaterSupported = mSafeStep;
        }

        while(!mBreakDebugger)
        {
            //wait for a debug event
            mIsRunning = true;
            if(!MyWaitForDebugEvent(&mDebugEvent, 100))
            {
                if(mDetach)
                {
                    if(!UnsafeDetach())
                        cbInternalError("Debugger::Detach failed!");
                    break;
                }
#if 0
                // Fix based on work by https://github.com/number201724
                if(WaitForSingleObject(mMainProcess.hProcess, 0) == WAIT_OBJECT_0)
                {
                    mDebugEvent.dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
                    mDebugEvent.dwProcessId = mMainProcess.dwProcessId;
                    mDebugEvent.dwThreadId = mMainProcess.dwThreadId;
                    if(!GetExitCodeProcess(mMainProcess.hProcess, &mDebugEvent.u.ExitProcess.dwExitCode))
                        mDebugEvent.u.ExitProcess.dwExitCode = 0xFFFFFFFF;
                }
#endif
                else
                {
                    // Regular timeout, wait again
                    continue;
                }
            }

            // Handle safe stepping
            if(IsDbgReplyLaterSupported)
            {
                if(mDebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
                {
                    // Check if there is a thread processing a single step
                    if(ThreadBeingProcessed != 0 && mDebugEvent.dwThreadId != ThreadBeingProcessed)
                    {
                        // Reply to the event later
                        if(!ContinueDebugEvent(mDebugEvent.dwProcessId, mDebugEvent.dwThreadId, DBG_REPLY_LATER))
                            break;

                        // Wait for the next event
                        continue;
                    }
                }
                else if(mDebugEvent.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
                {
                    if(ThreadBeingProcessed != 0 && mDebugEvent.dwThreadId == ThreadBeingProcessed)
                    {
                        // Resume the other threads since the thread being processed is exiting
                        for(auto & itr : SuspendedThreads)
                            ResumeThread(itr.second);

                        SuspendedThreads.clear();
                        ThreadBeingProcessed = 0;
                    }
                }
            }

            // Signal we are currently paused
            mIsRunning = false;

            //set default continue status
            mContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

            //set the current process and thread
            auto processFound = mProcesses.find(mDebugEvent.dwProcessId);
            if(processFound != mProcesses.end())
            {
                mProcess = processFound->second.get();
                auto threadFound = mProcess->threads.find(mDebugEvent.dwThreadId);
                if(threadFound != mProcess->threads.end())
                {
                    mThread = mProcess->thread = threadFound->second.get();
                }
                else
                {
                    mThread = mProcess->thread = nullptr;
                }
            }
            else
            {
                mThread = nullptr;
                if(mProcess)
                {
                    mProcess->thread = nullptr;
                    mProcess = nullptr;
                }
            }

            //call the pre debug event callback
            cbPreDebugEvent(mDebugEvent);

            //dispatch the debug event (documented here: https://msdn.microsoft.com/en-us/library/windows/desktop/ms679302(v=vs.85).aspx)
            switch(mDebugEvent.dwDebugEventCode)
            {
            case CREATE_PROCESS_DEBUG_EVENT:
                // HACK: when hollowing the process the debug event still delivers the original image base
                if(mDisableAslr && mDebugModuleImageBase != 0)
                {
                    auto startAddress = ULONG_PTR(mDebugEvent.u.CreateProcessInfo.lpStartAddress);
                    if(startAddress)
                    {
                        startAddress -= ULONG_PTR(mDebugEvent.u.CreateProcessInfo.lpBaseOfImage);
                        startAddress += mDebugModuleImageBase;
                        mDebugEvent.u.CreateProcessInfo.lpStartAddress = LPTHREAD_START_ROUTINE(startAddress);
                    }
                    mDebugEvent.u.CreateProcessInfo.lpBaseOfImage = LPVOID(mDebugModuleImageBase);
                }
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
                if(IsDbgReplyLaterSupported && mDebugEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP)
                {
                    // Resume the other threads since we are done processing the single step
                    for(auto & itr : SuspendedThreads)
                        ResumeThread(itr.second);

                    SuspendedThreads.clear();
                    ThreadBeingProcessed = 0;
                }
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
            if(mDetachAndBreak)
            {
                if(!UnsafeDetachAndBreak())
                    cbInternalError("Debugger::DetachAndBreak failed!");
                break;
            }

            //clear trap flag when set by GleeBug (to prevent an EXCEPTION_SINGLE_STEP after detach)
            if(mDetach && mThread)
            {
                if(mThread->isInternalStepping || mThread->isSingleStepping)
                    Registers(mThread->hThread, CONTEXT_CONTROL).TrapFlag = false;
            }

            // Handle safe stepping
            if(IsDbgReplyLaterSupported && mDebugEvent.dwDebugEventCode != EXIT_THREAD_DEBUG_EVENT)
            {
                // If TF is set (single step), then suspend all the other threads
                if(mThread && mThread->isInternalStepping)
                {
                    ThreadBeingProcessed = mDebugEvent.dwThreadId;

                    for(auto & Thread : mProcess->threads)
                    {
                        auto dwThreadId = Thread.first;
                        auto hThread = Thread.second->hThread;

                        // Do not suspend the current thread
                        if(ThreadBeingProcessed == dwThreadId)
                            continue;

                        // Check if the thread is already suspended
                        if(SuspendedThreads.count(dwThreadId) != 0)
                            continue;

                        if(SuspendThread(hThread) != -1)
                            SuspendedThreads.emplace(dwThreadId, hThread);
                    }
                }
            }

            //continue the debug event
            if(!ContinueDebugEvent(mDebugEvent.dwProcessId, mDebugEvent.dwThreadId, mContinueStatus))
                break;

            if(mDetach || mDetachAndBreak)
            {
                if(!UnsafeDetach())
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