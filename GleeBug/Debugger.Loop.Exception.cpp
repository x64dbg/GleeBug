#include "Debugger.h"

namespace GleeBug
{
    void Debugger::exceptionBreakpoint(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (!mProcess->systemBreakpoint) //handle system breakpoint
        {
            //set internal state
            mProcess->systemBreakpoint = true;
            mContinueStatus = DBG_CONTINUE;

            //get process DEP policy
#ifndef _WIN64
            typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
                _In_  HANDLE  /*hProcess*/,
                _Out_ LPDWORD /*lpFlags*/,
                _Out_ PBOOL   /*lpPermanent*/
                );
            static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));
            if (GPDP)
            {
                DWORD lpFlags;
                BOOL bPermanent;
                if (GPDP(mProcess->hProcess, &lpFlags, &bPermanent))
                    mProcess->permanentDep = lpFlags && bPermanent;
            }
#else
            mProcess->permanentDep = true;
#endif //_WIN64

            //call the attach callback if appropriate
            if(mAttachedToProcess && mProcess->dwProcessId == mMainProcess.dwProcessId)
                cbAttachBreakpoint();

            //call the callback
            cbSystemBreakpoint();
        }
        else
        {
            //check if the breakpoint exists
            auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Software, ptr(exceptionRecord.ExceptionAddress) });
            if (foundInfo == mProcess->breakpoints.end())
                return;
            const auto info = foundInfo->second;

            if (!info.enabled)
                return; //not a valid software breakpoint

            //set continue status
            mContinueStatus = DBG_CONTINUE;

            //set back the instruction pointer
            mRegisters->Gip = info.address;

            //restore the original breakpoint byte and do an internal step
            mProcess->MemWriteUnsafe(info.address, info.internal.software.oldbytes, info.internal.software.size);
            mThread->StepInternal(std::bind([this, info]()
            {
                //only restore the bytes if the breakpoint still exists
                if (mProcess->breakpoints.find({ BreakpointType::Software, info.address }) != mProcess->breakpoints.end())
                    mProcess->MemWriteUnsafe(info.address, info.internal.software.newbytes, info.internal.software.size);
            }));

            //call the generic callback
            cbBreakpoint(info);

            //call the user callback
            auto foundCallback = mProcess->breakpointCallbacks.find({ BreakpointType::Software, info.address });
            if (foundCallback != mProcess->breakpointCallbacks.end())
                foundCallback->second(info);

            //delete the breakpoint if it is singleshoot
            if (info.singleshoot)
                mProcess->DeleteGenericBreakpoint(info);
        }
    }

    void Debugger::exceptionSingleStep(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (mThread->isInternalStepping) //handle internal steps
        {
            //set internal status
            mThread->isInternalStepping = false;
            mContinueStatus = DBG_CONTINUE;

            //call the internal step callback
            mThread->cbInternalStep();
        }
        if (mThread->isSingleStepping) //handle single step
        {
            //set internal status
            mThread->isSingleStepping = false;
            mContinueStatus = DBG_CONTINUE;

            //call the generic callback
            cbStep();

            //call the user callbacks
            auto cbStepCopy = mThread->stepCallbacks;
            mThread->stepCallbacks.clear();
            for (auto cbStep : cbStepCopy)
                cbStep();
        }
        else //handle hardware breakpoint single step exceptions
        {
            exceptionHardwareBreakpoint(ptr(exceptionRecord.ExceptionAddress));
        }
    }

    void Debugger::exceptionHardwareBreakpoint(ptr exceptionAddress)
    {
        //determine the hardware breakpoint triggered
        ptr dr6 = mRegisters->Dr6();
        HardwareSlot breakpointSlot;
        ptr breakpointAddress;
        if (exceptionAddress == mRegisters->Dr0() || dr6 & 0x1)
        {
            breakpointAddress = mRegisters->Dr0();
            breakpointSlot = HardwareSlot::Dr0;
        }
        else if (exceptionAddress == mRegisters->Dr1() || dr6 & 0x2)
        {
            breakpointAddress = mRegisters->Dr1();
            breakpointSlot = HardwareSlot::Dr1;
        }
        else if (exceptionAddress == mRegisters->Dr2() || dr6 & 0x4)
        {
            breakpointAddress = mRegisters->Dr2();
            breakpointSlot = HardwareSlot::Dr2;
        }
        else if (exceptionAddress == mRegisters->Dr3() || dr6 & 0x8)
        {
            breakpointAddress = mRegisters->Dr3();
            breakpointSlot = HardwareSlot::Dr3;
        }
        else
            return; //not a hardware breakpoint

        //find the breakpoint in the internal structures
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Hardware, breakpointAddress });
        if (foundInfo == mProcess->breakpoints.end())
            return; //not a valid hardware breakpoint
        const auto info = foundInfo->second;
        if (info.internal.hardware.slot != breakpointSlot || !info.enabled)
            return; //not a valid hardware breakpoint

        //set continue status
        mContinueStatus = DBG_CONTINUE;

        //delete the hardware breakpoint from the thread (not the breakpoint buffer) and do an internal step (TODO: maybe delete from all threads?)
        mThread->DeleteHardwareBreakpoint(breakpointSlot);
        mThread->StepInternal(std::bind([this, info]()
        {
            //only restore if the breakpoint still exists
            if (mProcess->breakpoints.find({ BreakpointType::Hardware, info.address }) != mProcess->breakpoints.end())
                mThread->SetHardwareBreakpoint(info.address, info.internal.hardware.slot, info.internal.hardware.type, info.internal.hardware.size);
        }));

        //call the generic callback
        cbBreakpoint(info);

        //call the user callback
        auto foundCallback = mProcess->breakpointCallbacks.find({ BreakpointType::Hardware, info.address });
        if (foundCallback != mProcess->breakpointCallbacks.end())
            foundCallback->second(info);

        //delete the breakpoint if it is singleshoot
        if (info.singleshoot)
            mProcess->DeleteGenericBreakpoint(info);
    }

    void Debugger::exceptionGuardPage(const EXCEPTION_RECORD & exceptionRecord, bool firstChance)
    {
        char error[128] = "";
        auto exceptionAddress = ptr(exceptionRecord.ExceptionInformation[1]);

        //check if the exception address is directly in the range of a memory breakpoint
        auto foundRange = mProcess->memoryBreakpointRanges.find(Range(exceptionAddress, exceptionAddress));
        if (foundRange == mProcess->memoryBreakpointRanges.end())
        {
            //if not in range, check if a memory breakpoint is in the accessed page
            auto foundPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
            if (foundPage != mProcess->memoryBreakpointPages.end())
            {
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;
                //TODO: single step and page protection changes
                //FIXED
                if (!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", foundPage->first);
                    cbInternalError(error);
                }
                //step + restore new protection to keep bp 
                mThread->StepInternal(std::bind([this, page, pBaseAddr]()
                {
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, page.NewProtect);
                    return;
                }));
            }
            return;
        }

        //find the breakpoint associated with the hit breakpoint range
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if (foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", exceptionAddress);
            cbInternalError(error);
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;
        if (!info.enabled)
            return;

        printf("memory breakpoint: 0x%p (size: %d)\n", info.address, info.internal.memory.size);

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED: Marques
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        if (bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", exceptionAddress & ~(PAGE_SIZE - 1));
            cbInternalError(error);
            return;
        }
        /*
        Access = 1,
        Read = 2,
        Write = 4,
        Execute = 8
        */
        //Read but our bpx page is not bp on Read
        //We shouldn't care about other stuff such as Write or Execute since these breakpoints are implemented with Access Violation.
        if ((exceptionRecord.ExceptionInformation[0]==0) && (!(bpxPage->second.Type & 0x2)))
        {
            //perhaps the program generated such exception
            return;
        }

        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED: Marques
        auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if (bpxCb != mProcess->breakpointCallbacks.end())
        {
            bpxCb->second(info);
        }


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if (!mProcess->MemProtect(bpxPage->first, PAGE_SIZE, bpxPage->second.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", bpxPage->first);
            cbInternalError(error);
        }
        //Pass info as well
        auto pageAddr = bpxPage->first;
        auto pageProperties = bpxPage->second;
        mThread->StepInternal(std::bind([this, info, pageAddr, pageProperties]()
        {
            //Check if the bpx still exists
            auto found_range = mProcess->memoryBreakpointRanges.find(Range(info.address, info.address));
            if (found_range != mProcess->memoryBreakpointRanges.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.NewProtect);
            }
            return;
        }));

        if (foundInfo->second.singleshoot)
        {
            mProcess->DeleteMemoryBreakpoint(exceptionAddress);
        }

    }


    void Debugger::exceptionAccessViolation(const EXCEPTION_RECORD & exceptionRecord, bool firstChance)
    {
        //Same shit as before

        char error[128] = "";
        auto exceptionAddress = ptr(exceptionRecord.ExceptionInformation[1]);

        //check if the exception address is directly in the range of a memory breakpoint
        auto foundRange = mProcess->memoryBreakpointRanges.find(Range(exceptionAddress, exceptionAddress));
        if (foundRange == mProcess->memoryBreakpointRanges.end())
        {
            //if not in range, check if a memory breakpoint is in the accessed page
            auto foundPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
            if (foundPage != mProcess->memoryBreakpointPages.end())
            {
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;
                //TODO: single step and page protection changes
                //FIXED
                if (!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", foundPage->first);
                    cbInternalError(error);
                }
                //step + restore new protection to keep bp 
                mThread->StepInternal(std::bind([this, page, pBaseAddr]()
                {
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, page.NewProtect);
                    return;
                }));
            }
            return;
        }

        //find the breakpoint associated with the hit breakpoint range
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if (foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", exceptionAddress);
            cbInternalError(error);
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;
        if (!info.enabled)
            return;

        printf("memory breakpoint: 0x%p (size: %d)\n", info.address, info.internal.memory.size);

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED: Marques
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        if (bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", exceptionAddress & ~(PAGE_SIZE - 1));
            cbInternalError(error);
            return;
        }
        /*
        Access = 1,
        Read = 2,
        Write = 4,
        Execute = 8
        */
        //Write but the bpx was not on write
        //We shouldn't care about other stuff such as read or access because those were implemented with page guards.
        if ((exceptionRecord.ExceptionInformation[0] == 1) && (!(bpxPage->second.Type & 0x4)))
        {
            //perhaps the program generated such exception
            return;
        }

        //Exec but bpx was not on exec.
        if ((exceptionRecord.ExceptionInformation[0] == 8) && (!(bpxPage->second.Type & 0x8)))
        {
            //perhaps the program generated such exception
            return;
        }
        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED: Marques
        auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if (bpxCb != mProcess->breakpointCallbacks.end())
        {
            bpxCb->second(info);
        }


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if (!mProcess->MemProtect(bpxPage->first, PAGE_SIZE, bpxPage->second.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", bpxPage->first);
            cbInternalError(error);
        }
        //Pass info as well
        auto pageAddr = bpxPage->first;
        auto pageProperties = bpxPage->second;
        mThread->StepInternal(std::bind([this, info, pageAddr, pageProperties]()
        {
            //Check if the bpx still exists
            auto found_range = mProcess->memoryBreakpointRanges.find(Range(info.address, info.address));
            if (found_range != mProcess->memoryBreakpointRanges.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.NewProtect);
            }
            return;
        }));

        if (foundInfo->second.singleshoot)
        {
            mProcess->DeleteMemoryBreakpoint(exceptionAddress);
        }
    }

    void Debugger::exceptionEvent(const EXCEPTION_DEBUG_INFO & exceptionInfo)
    {
        //let the debuggee handle exceptions per default
        mContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

        const EXCEPTION_RECORD & exceptionRecord = exceptionInfo.ExceptionRecord;
        bool firstChance = exceptionInfo.dwFirstChance == 1;

        //call the debug event callback
        cbExceptionEvent(exceptionInfo);

        //dispatch the exception (https://msdn.microsoft.com/en-us/library/windows/desktop/aa363082(v=vs.85).aspx)
        switch (exceptionInfo.ExceptionRecord.ExceptionCode)
        {
        case STATUS_BREAKPOINT:
            exceptionBreakpoint(exceptionRecord, firstChance);
            break;
        case STATUS_SINGLE_STEP:
            exceptionSingleStep(exceptionRecord, firstChance);
            break;
        case STATUS_GUARD_PAGE_VIOLATION:
            exceptionGuardPage(exceptionRecord, firstChance);
            break;
        case STATUS_ACCESS_VIOLATION:
            exceptionAccessViolation(exceptionRecord, firstChance);
            break;
        }

        //call the unhandled exception callback
        if (mContinueStatus == DBG_EXCEPTION_NOT_HANDLED)
            cbUnhandledException(exceptionRecord, firstChance);
    }
};