#include "Debugger.h"
#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
    void Debugger::exceptionBreakpoint(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        //check if the breakpoint exists
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Software, ptr(exceptionRecord.ExceptionAddress) });
        if(foundInfo == mProcess->breakpoints.end())
        {
            if(!this->mAttachedToProcess && !mProcess->systemBreakpoint) //handle system breakpoint
            {
                //set internal state
                mProcess->systemBreakpoint = true;
                mContinueStatus = DBG_CONTINUE;

                //get process DEP policy (TODO: what happens if a breakpoint is hit before the system breakpoint?)
#ifndef _WIN64
                typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
                    _In_  HANDLE  /*hProcess*/,
                    _Out_ LPDWORD /*lpFlags*/,
                    _Out_ PBOOL   /*lpPermanent*/
                    );
                static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));
                if(GPDP)
                {
                    //If you use mProcess->hProcess GetProcessDEPPolicy will put garbage in bPermanent.
                    auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mProcess->dwProcessId);
                    DWORD lpFlags;
                    BOOL bPermanent;
                    if(GPDP(hProcess, &lpFlags, &bPermanent))
                        mProcess->permanentDep = lpFlags != 0 && bPermanent;
                    CloseHandle(hProcess);
                }
#else
                mProcess->permanentDep = true;
#endif //_WIN64

                //call the callback
                cbSystemBreakpoint();
            }
            return;
        }

        const auto info = foundInfo->second;

        //set continue status
        mContinueStatus = DBG_CONTINUE;

        //set back the instruction pointer
        Registers(mThread->hThread, CONTEXT_CONTROL).Gip = info.address;

        //restore the original breakpoint byte and do an internal step
        mProcess->MemWriteUnsafe(info.address, info.internal.software.oldbytes, info.internal.software.size);
        mThread->StepInternal(std::bind([this, info]()
        {
            //only restore the bytes if the breakpoint still exists
            if(mProcess->breakpoints.find({ BreakpointType::Software, info.address }) != mProcess->breakpoints.end())
                mProcess->MemWriteUnsafe(info.address, info.internal.software.newbytes, info.internal.software.size);
        }));

        //call the generic callback
        cbBreakpoint(info);

        //call the user callback
        auto foundCallback = mProcess->breakpointCallbacks.find({ BreakpointType::Software, info.address });
        if(foundCallback != mProcess->breakpointCallbacks.end())
            foundCallback->second(info);

        //delete the breakpoint if it is singleshoot
        if(info.singleshoot)
            mProcess->DeleteGenericBreakpoint(info);
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
        Registers registers(mThread->hThread, CONTEXT_DEBUG_REGISTERS);
        ptr dr6 = registers.Dr6();
        HardwareSlot breakpointSlot;
        ptr breakpointAddress;
        if (exceptionAddress == registers.Dr0() || dr6 & 0x1)
        {
            breakpointAddress = registers.Dr0();
            breakpointSlot = HardwareSlot::Dr0;
        }
        else if (exceptionAddress == registers.Dr1() || dr6 & 0x2)
        {
            breakpointAddress = registers.Dr1();
            breakpointSlot = HardwareSlot::Dr1;
        }
        else if (exceptionAddress == registers.Dr2() || dr6 & 0x4)
        {
            breakpointAddress = registers.Dr2();
            breakpointSlot = HardwareSlot::Dr2;
        }
        else if (exceptionAddress == registers.Dr3() || dr6 & 0x8)
        {
            breakpointAddress = registers.Dr3();
            breakpointSlot = HardwareSlot::Dr3;
        }
        else
            return; //not a hardware breakpoint

        //find the breakpoint in the internal structures
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Hardware, breakpointAddress });
        if (foundInfo == mProcess->breakpoints.end())
            return; //not a valid hardware breakpoint
        const auto info = foundInfo->second;
        if (info.internal.hardware.slot != breakpointSlot)
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
        /*
        ASSUME:
        exceptionAddress may or may not have been generated by your breakpoints.
        */
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
                //(this means that by our fault the program generated an exception, we should clean it)
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;

                //We restore the protection
                if (!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", (void*)foundPage->first);
                    cbInternalError(error);
                }

                //However the following situations may occur:
                // The instruction we singlestep to is a software breakpoint, which may execute a callback, that can :
                // -actually delete a memory breakpoint that takes this page into account 
                // -add more memory breakpoints
                //The solution: We just try to see if the page is mapped into memoryBreakpointPages. If the page is in deed being used by any memory breakpoint,
                // then we ought to restore the protection.
                mThread->StepInternal(std::bind([this, pBaseAddr]()
                {
                    //seek out the page address
                    auto found_page = mProcess->memoryBreakpointPages.find(pBaseAddr);
                    if (found_page == mProcess->memoryBreakpointPages.end())
                    {
                        //no page being used by bpx? Then just return
                        return;
                    }
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, found_page->second.NewProtect);
                    return;
                }));
            }
            return;
        }

        /*
        ASSUME:
        exceptionAddress is indeed inside a breakpoint range you have defined.
        */
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if (foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", (void*)exceptionAddress);
            cbInternalError(error);
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED: 
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        auto pageAddr = bpxPage->first;
        auto pageProperties = bpxPage->second;

        if (bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", (void*)(exceptionAddress & ~(PAGE_SIZE - 1)));
            cbInternalError(error);
            return;
        }

        //TODO: If I only have a page with Read bp and the exception was not on read, I don't execute the callback. Because since this was implemented with PAGE_GUARD, writtes or executes still trigger
        //This callback.
        //FIX: If the memoryBreakpointPages for this page does not have a access flag and has a read flag, but the exception was not on read. Then we resume the debuggee.
        if ((exceptionRecord.ExceptionInformation[0] != 0))
        {
            //The bpx is solely on read.
            if (((pageProperties.Type & 0x2) != 0) && ((pageProperties.Type & 0x1) == 0))
               {
                   mContinueStatus = DBG_CONTINUE;
                    //We restore the protection
                    if (!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
                    {
                        sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
                        cbInternalError(error);
                    }

                    mThread->StepInternal(std::bind([this, pageAddr]()
                    {
                        //seek out the page address
                        auto found_page = mProcess->memoryBreakpointPages.find(pageAddr);
                        if (found_page == mProcess->memoryBreakpointPages.end())
                        {
                            //no page being used by bpx? Then just return
                            return;
                        }
                        mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
                        return;
                    }));
                    return;
                }
            else if (((pageProperties.Type & 0x1) != 0))
            {
                //We are fine if the breakpoint is on Access and somethine other than a read occurred.
            }
            else
            {
                //This exception handler was called within a page that had no breakpoints on read or access. Probably the program generated this exception! what a 0x1337 brat.
                //In this situation we return control to debuggee.
                return;
            }

        }
        else
        {
            //The generated exception is on read.
            //If the page doesn't have a breakpoint on read or on access then something else must have gone wrong - we pass execution to debuggee.
            if ((!(pageProperties.Type & 0x2)) && (!(pageProperties.Type & 0x1)))
            {
                return;
            }
        }
        

        /*
        ASSUME:
        The breakpoint at exceptionAddress was indeed generated by me.
        Its safe to call the callbacks.
        */
        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED:
        auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if (bpxCb != mProcess->breakpointCallbacks.end())
        {
            bpxCb->second(info);
        }


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if (!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
            cbInternalError(error);
        }
        //Pass info as well
        mThread->StepInternal(std::bind([this, pageAddr]()
        {
            //With page check this should work better: So when we reach this part of the code we are sure that:
            //-The exception Address In deed corresponded to an existing (now possibly deleted) memory breakpoint range
            //-memoryBreakpointPages was in deed consistent with this memory address that generated the exception (The data structure wasn't corrupted somehow)
            //So our new technique basically checks if the page address is still inside memoryBreakpointRanges structure. If this is true, we simply apply the NewProtect.
            //Wide variety of possible scenarios:
            //-Bpx on this page and bpx is not singleshot: In the case of PAGE_GUARD page protection (handled by this exception handler), if the page permission map persists, we simply 
            // enforce the newProtect because this page belongs to a breakpoint somewhere.
            //-Bpx is singleshot: Then it was deleted by the end of this call. If the refcount is zero, then we dont find the page on the Memory map, so assume no more memory breakpoints happen there.
            // therefore, we do not enforce new protection.
            //-Bpx was deleted on the handler: Again the page may or may not be mapped on memoryBreakpointPages. If the bp was deleted, and there are no more breakpoints in this page - The page does not exist on the map and therefore we do not restore old page protection.
            //-Bpx was deleted on the handler AND a new breakpoint was added: if the bpx was deleted, and a new one was added on this page then, surely the page is mapped under memoryBreakpointPages.
            //Check if the memory page is mapped

            auto found_page = mProcess->memoryBreakpointPages.find(pageAddr);
            if (found_page != mProcess->memoryBreakpointPages.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
            }
            return;
        }));

        if (info.singleshoot)
        {
            mProcess->DeleteMemoryBreakpoint(exceptionAddress);
        }

    }


    void Debugger::exceptionAccessViolation(const EXCEPTION_RECORD & exceptionRecord, bool firstChance)
    {
        /*
        ASSUME:
        exceptionAddress may or may not have been generated by your breakpoints.
        */
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
                //(this means that by our fault the program generated an exception, we should clean it)
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;

                //We restore the protection
                if (!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", (void*)foundPage->first);
                    cbInternalError(error);
                }

                //However the following situations may occur:
                // The instruction we singlestep to is a software breakpoint, which may execute a callback, that can :
                // -actually delete a memory breakpoint that takes this page into account 
                // -add more memory breakpoints
                //The solution: We just try to see if the page is mapped into memoryBreakpointPages. If the page is in deed being used by any memory breakpoint,
                // then we ought to restore the protection.
                mThread->StepInternal(std::bind([this, pBaseAddr]()
                {
                    //seek out the page address
                    auto found_page = mProcess->memoryBreakpointPages.find(pBaseAddr);
                    if (found_page == mProcess->memoryBreakpointPages.end())
                    {
                        //no page being used by bpx? Then just return
                        return;
                    }
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, found_page->second.NewProtect);
                    return;
                }));
            }
            return;
        }

        /*
        ASSUME:
        exceptionAddress is indeed inside a breakpoint range you have defined.
        */
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if (foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", (void*)exceptionAddress);
            cbInternalError(error);
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED: 
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        if (bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", (void*)(exceptionAddress & ~(PAGE_SIZE - 1)));
            cbInternalError(error);
            return;
        }
        auto pageAddr = bpxPage->first;
        auto pageProperties = bpxPage->second;

        /*
        Access = 1,
        Read = 2,
        Write = 4,
        Execute = 8
        */
        //ExceptionInformation[0] should be considered as 1 or 8, because these are for exceptions generated on write or execute.
        //Execute is only implemented with page guard if no Data-Execution-Prevention is implemented by the Kernel.
        if ((exceptionRecord.ExceptionInformation[0] == 1) && (!(pageProperties.Type & 4)))
        {
            //The exception was on Write but there was no page breakpoint in Write? Then the program changed the page permissions, or naturally overwritten protected data. We do not interfere.
            return;
        }

        if ((exceptionRecord.ExceptionInformation[0] == 8) && (!(pageProperties.Type & 8)))
        {
            //The exception was on Execution but there was no page breakpoint in Execute? Then the program changed the page permissions, or naturally executed protected code. We do not interfere.
            return;
        }
        /*
        ASSUME:
        The breakpoint at exceptionAddress was indeed generated by me.
        */
        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED:
        auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if (bpxCb != mProcess->breakpointCallbacks.end())
        {
            bpxCb->second(info);
        }


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if (!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
            cbInternalError(error);
        }
        //Pass info as well
        mThread->StepInternal(std::bind([this, pageAddr]()
        {
            //With page check this should work better: So when we reach this part of the code we are sure that:
            //-The exception Address In deed corresponded to an existing (now possibly deleted) memory breakpoint range
            //-memoryBreakpointPages was in deed consistent with this memory address that generated the exception (The data structure wasn't corrupted somehow)
            //So our new technique basically checks if the page address is still inside memoryBreakpointRanges structure. If this is true, we simply apply the NewProtect.
            //Wide variety of possible scenarios:
            //-Bpx on this page and bpx is not singleshot: In the case of PAGE_GUARD page protection (handled by this exception handler), if the page permission map persists, we simply 
            // enforce the newProtect because this page belongs to a breakpoint somewhere.
            //-Bpx is singleshot: Then it was deleted by the end of this call. If the refcount is zero, then we dont find the page on the Memory map, so assume no more memory breakpoints happen there.
            // therefore, we do not enforce new protection.
            //-Bpx was deleted on the handler: Again the page may or may not be mapped on memoryBreakpointPages. If the bp was deleted, and there are no more breakpoints in this page - The page does not exist on the map and therefore we do not restore old page protection.
            //-Bpx was deleted on the handler AND a new breakpoint was added: if the bpx was deleted, and a new one was added on this page then, surely the page is mapped under memoryBreakpointPages.
            //Check if the memory page is mapped

            auto found_page = mProcess->memoryBreakpointPages.find(pageAddr);
            if (found_page != mProcess->memoryBreakpointPages.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
            }
            return;
        }));

        if (info.singleshoot)
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