#include "Debugger.Thread.h"

namespace GleeBug
{
    Thread::Thread(HANDLE hThread, uint32 dwThreadId, LPVOID lpThreadLocalBase, LPVOID lpStartAddress) :
        hThread(hThread),
        dwThreadId(dwThreadId),
        lpThreadLocalBase(ptr(lpThreadLocalBase)),
        lpStartAddress(ptr(lpStartAddress)),
        isSingleStepping(false),
        isInternalStepping(false),
        cbInternalStep(nullptr)
    {
    }

    bool Thread::RegReadContext()
    {
        memset(&this->mOldContext, 0, sizeof(CONTEXT));
        this->mOldContext.ContextFlags = CONTEXT_ALL; //TODO: granular control over what's required
        if(GetThreadContext(this->hThread, &this->mOldContext))
        {
            this->registers.SetContext(this->mOldContext);
            return true;
        }
        __debugbreak();
        return false;
    }

    bool Thread::RegWriteContext()
    {
        //check if something actually changed
        if (memcmp(&this->mOldContext, &this->registers.mContext, sizeof(CONTEXT)) == 0)
            return true;
        //update the context
        if(SetThreadContext(this->hThread, &this->registers.mContext))
            return true;
        __debugbreak();
        return false;
    }

    void Thread::StepInto()
    {
        registers.TrapFlag.Set();
        isSingleStepping = true;
    }

    void Thread::StepInto(const StepCallback & cbStep)
    {
        StepInto();
        
        auto target = cbStep.target<void()>();
        for (const auto & cb : stepCallbacks)
        {
            if (target == cb.target<void()>())
            {
                puts("duplicate StepInto callback detected!");
                return;
            }
        }
        stepCallbacks.push_back(cbStep);
    }

    void Thread::StepInternal(const StepCallback & cbStep)
    {
        registers.TrapFlag.Set();
        isInternalStepping = true;
        cbInternalStep = cbStep;
    }

    bool Thread::Suspend()
    {
        return SuspendThread(hThread) != -1;
    }

    bool Thread::Resume()
    {
        return ResumeThread(hThread) != -1;
    }
};