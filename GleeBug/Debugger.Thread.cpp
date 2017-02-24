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

    Thread::Thread(const Thread & other) :
        hThread(other.hThread),
        dwThreadId(other.dwThreadId),
        lpThreadLocalBase(other.lpThreadLocalBase),
        lpStartAddress(other.lpStartAddress),
        registers(), //create new registers
        stepCallbacks(other.stepCallbacks),
        isSingleStepping(other.isSingleStepping),
        isInternalStepping(other.isInternalStepping),
        cbInternalStep(other.cbInternalStep)
    {
    }

    Thread & Thread::operator=(const Thread& other)
    {
        hThread = other.hThread;
        dwThreadId = other.dwThreadId;
        lpThreadLocalBase = other.lpThreadLocalBase;
        lpStartAddress = other.lpStartAddress;
        registers = Registers(); //create new registers
        stepCallbacks = other.stepCallbacks;
        isSingleStepping = other.isSingleStepping;
        isInternalStepping = other.isInternalStepping;
        cbInternalStep = other.cbInternalStep;
        return *this;
    }

    bool Thread::RegReadContext()
    {
        memset(&this->mOldContext, 0, sizeof(CONTEXT));
        this->mOldContext.ContextFlags = CONTEXT_ALL; //TODO: granular control over what's required
        this->registers.setContextLazy(&this->mOldContext, this->hThread);
        return true;
    }

    bool Thread::RegWriteContext()
    {
        //check if something actually changed
        if (memcmp(&this->mOldContext, &this->registers.mContext, sizeof(CONTEXT)) == 0)
            return true;
        //update the context
        return !!SetThreadContext(this->hThread, &this->registers.mContext);
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
};