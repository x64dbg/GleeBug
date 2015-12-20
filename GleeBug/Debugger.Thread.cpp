#include "Debugger.Thread.h"

namespace GleeBug
{
    ThreadInfo::ThreadInfo(HANDLE hThread, uint32 dwThreadId, LPVOID lpThreadLocalBase, LPVOID lpStartAddress) :
        hThread(hThread),
        dwThreadId(dwThreadId),
        lpThreadLocalBase(ptr(lpThreadLocalBase)),
        lpStartAddress(ptr(lpStartAddress)),
        isSingleStepping(false),
        isInternalStepping(false),
        cbInternalStep(nullptr)
    {
    }

    ThreadInfo::ThreadInfo(const ThreadInfo & other) :
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

    ThreadInfo & ThreadInfo::operator=(const ThreadInfo& other)
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

    bool ThreadInfo::RegReadContext()
    {
        SuspendThread(this->hThread);
        memset(&this->_oldContext, 0, sizeof(CONTEXT));
        this->_oldContext.ContextFlags = CONTEXT_ALL;
        bool bReturn = false;
        if (GetThreadContext(this->hThread, &this->_oldContext))
        {
            this->registers.SetContext(this->_oldContext);
            bReturn = true;
        }
        ResumeThread(this->hThread);
        return bReturn;
    }

    bool ThreadInfo::RegWriteContext() const
    {
        //check if something actually changed
        if (memcmp(&this->_oldContext, this->registers.GetContext(), sizeof(CONTEXT)) == 0)
            return true;
        //update the context
        SuspendThread(this->hThread);
        bool bReturn = !!SetThreadContext(this->hThread, this->registers.GetContext());
        ResumeThread(this->hThread);
        return bReturn;
    }

    void ThreadInfo::StepInto()
    {
        registers.TrapFlag.Set();
        isSingleStepping = true;
    }

    void ThreadInfo::StepInto(const StepCallback & cbStep)
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

    void ThreadInfo::StepInternal(const StepCallback & cbStep)
    {
        registers.TrapFlag.Set();
        isInternalStepping = true;
        cbInternalStep = cbStep;
    }
};