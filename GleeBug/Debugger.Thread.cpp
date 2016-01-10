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
        SuspendThread(this->hThread);
        memset(&this->mOldContext, 0, sizeof(CONTEXT));
        this->mOldContext.ContextFlags = CONTEXT_ALL;
        bool bReturn = false;
        if (GetThreadContext(this->hThread, &this->mOldContext))
        {
            this->registers.SetContext(this->mOldContext);
            bReturn = true;
        }
        ResumeThread(this->hThread);
        return bReturn;
    }

    bool Thread::RegWriteContext() const
    {
        //check if something actually changed
        if (memcmp(&this->mOldContext, this->registers.GetContext(), sizeof(CONTEXT)) == 0)
            return true;
        //update the context
        SuspendThread(this->hThread);
        bool bReturn = !!SetThreadContext(this->hThread, this->registers.GetContext());
        ResumeThread(this->hThread);
        return bReturn;
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