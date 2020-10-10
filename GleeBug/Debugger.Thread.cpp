#include "Debugger.Thread.h"
#include "Debugger.Thread.Registers.h"

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

    void Thread::StepInto()
    {
        Registers(hThread).TrapFlag.Set();
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
        Registers(hThread).TrapFlag.Set();
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
