#include "Debugger.h"

namespace GleeBug
{
    void Debugger::createThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread)
    {
        //thread housekeeping
        mProcess->threads.insert({ mDebugEvent.dwThreadId,
            std::make_unique<Thread>(createThread.hThread,
            mDebugEvent.dwThreadId,
            createThread.lpThreadLocalBase,
            createThread.lpStartAddress) });

        //set the current thread
        mThread = mProcess->thread = mProcess->threads.find(mDebugEvent.dwThreadId)->second.get();
        mRegisters = &mThread->registers;
        if (!mThread->RegReadContext())
            cbInternalError("Thread::RegReadContext() failed!");

        //call the debug event callback
        cbCreateThreadEvent(createThread, *mThread);
    }

    void Debugger::exitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread)
    {
        //call the debug event callback
        cbExitThreadEvent(exitThread, *mThread);

        //thread housekeeping
        mProcess->threads.erase(mDebugEvent.dwThreadId);

        //set the current thread
        mThread = mProcess->thread = nullptr;
        mRegisters = nullptr;
    }
};