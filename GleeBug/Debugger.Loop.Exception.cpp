#include "Debugger.h"
#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
    void Debugger::exceptionBreakpoint(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        std::unique_lock<std::recursive_mutex> lock(mProcess->breakpointMutex);

        //check if the breakpoint exists
        auto exceptionAddress = ptr(exceptionRecord.ExceptionAddress);
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Software, exceptionAddress });
        if(foundInfo == mProcess->breakpoints.end())
        {
            if(!this->mAttachedToProcess && !mProcess->systemBreakpoint) //handle system breakpoint
            {
                //set internal state
                mProcess->systemBreakpoint = true;
                mContinueStatus = DBG_CONTINUE;

                //call the callback
                lock.unlock();
                cbSystemBreakpoint();
            }
            else
            {
                //check if this address had a breakpoint that was recently deleted
                auto & deletedBps = mProcess->recentlyDeletedSwbp;
                auto foundIt = deletedBps.find(exceptionAddress);
                if(foundIt != deletedBps.end() && mThread)
                {
                    Registers(mThread->hThread, CONTEXT_CONTROL).Gip = exceptionAddress;
                    mContinueStatus = DBG_CONTINUE;
                }
            }
            return;
        }

        const auto info = foundInfo->second;

        //set continue status
        mContinueStatus = DBG_CONTINUE;

        //set back the instruction pointer
        Registers(mThread->hThread, CONTEXT_CONTROL).Gip = info.address;

        //restore the original breakpoint byte and do an internal step
        if(!mProcess->MemWriteUnsafe(info.address, info.internal.software.oldbytes, info.internal.software.size))
        {
            //failed to restore original byte, pass exception to debuggee
            mContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
            return;
        }
        mProcess->StepInternal([this, info]()
        {
            std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

            //only restore the bytes if the breakpoint still exists
            auto foundBreakpoint = mProcess->breakpoints.find({ BreakpointType::Software, info.address });
            if(foundBreakpoint != mProcess->breakpoints.end())
            {
                if(!mProcess->MemWriteUnsafe(info.address, info.internal.software.newbytes, info.internal.software.size))
                {
                    //failed to restore breakpoint byte, remove from maps to stay consistent
                    mProcess->softwareBreakpointReferences.erase(info.address);
                    mProcess->breakpoints.erase(foundBreakpoint);
                    mProcess->breakpointCallbacks.erase({ BreakpointType::Software, info.address });
                }
            }
        });

        BreakpointCallback breakpointCallback;
        auto foundCallback = mProcess->breakpointCallbacks.find({ BreakpointType::Software, info.address });
        if(foundCallback != mProcess->breakpointCallbacks.end())
            breakpointCallback = foundCallback->second;
        lock.unlock();

        //call the generic callback
        cbBreakpoint(info);

        //call the user callback
        if(breakpointCallback)
            breakpointCallback(info);

        //delete the breakpoint if it is singleshoot
        if(info.singleshoot)
            mProcess->DeleteGenericBreakpoint(info);
    }

    void Debugger::exceptionSingleStep(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if(mThread->isInternalStepping)  //handle internal steps
        {
            //set internal status
            mThread->isInternalStepping = false;
            mContinueStatus = DBG_CONTINUE;

            mThread->cbInternalStep();
        }
        if(mThread->isSingleStepping)  //handle single step
        {
            //set internal status
            mThread->isSingleStepping = false;
            mContinueStatus = DBG_CONTINUE;

            //call the generic callback
            cbStep();

            //call the user callbacks
            auto cbStepCopy = mThread->stepCallbacks;
            mThread->stepCallbacks.clear();
            for(auto cbStep : cbStepCopy)
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
        if(exceptionAddress == registers.Dr0() || dr6 & 0x1)
        {
            breakpointAddress = registers.Dr0();
            breakpointSlot = HardwareSlot::Dr0;
        }
        else if(exceptionAddress == registers.Dr1() || dr6 & 0x2)
        {
            breakpointAddress = registers.Dr1();
            breakpointSlot = HardwareSlot::Dr1;
        }
        else if(exceptionAddress == registers.Dr2() || dr6 & 0x4)
        {
            breakpointAddress = registers.Dr2();
            breakpointSlot = HardwareSlot::Dr2;
        }
        else if(exceptionAddress == registers.Dr3() || dr6 & 0x8)
        {
            breakpointAddress = registers.Dr3();
            breakpointSlot = HardwareSlot::Dr3;
        }
        else
            return; //not a hardware breakpoint

        //find the breakpoint in the internal structures
        std::unique_lock<std::recursive_mutex> lock(mProcess->breakpointMutex);
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Hardware, breakpointAddress });
        if(foundInfo == mProcess->breakpoints.end())
            return; //not a valid hardware breakpoint
        const auto info = foundInfo->second;
        if(info.internal.hardware.slot != breakpointSlot)
            return; //not a valid hardware breakpoint

        //set continue status
        mContinueStatus = DBG_CONTINUE;

        //delete the hardware breakpoint from the thread (not the breakpoint buffer) and do an internal step (TODO: maybe delete from all threads?)
        mThread->DeleteHardwareBreakpoint(breakpointSlot);
        mProcess->StepInternal([this, info]()
        {
            std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

            //only restore if the breakpoint still exists
            if(mProcess->breakpoints.find({ BreakpointType::Hardware, info.address }) != mProcess->breakpoints.end())
                mThread->SetHardwareBreakpoint(info.address, info.internal.hardware.slot, info.internal.hardware.type, info.internal.hardware.size);
        });

        BreakpointCallback breakpointCallback;
        auto foundCallback = mProcess->breakpointCallbacks.find({ BreakpointType::Hardware, info.address });
        if(foundCallback != mProcess->breakpointCallbacks.end())
            breakpointCallback = foundCallback->second;
        lock.unlock();

        //call the generic callback
        cbBreakpoint(info);

        //call the user callback
        if(breakpointCallback)
            breakpointCallback(info);

        //if the breakpoint was deleted during callback, clear internal stepping to prevent thread suspension
        lock.lock();
        const bool breakpointDeleted = mProcess->breakpoints.find({ BreakpointType::Hardware, info.address }) == mProcess->breakpoints.end();
        lock.unlock();
        if(breakpointDeleted)
        {
            mThread->isInternalStepping = false;
            Registers(mThread->hThread, CONTEXT_CONTROL).TrapFlag = false;
        }

        //delete the breakpoint if it is singleshoot
        if(info.singleshoot)
            mProcess->DeleteGenericBreakpoint(info);
    }

    void Debugger::exceptionGuardPage(const EXCEPTION_RECORD & exceptionRecord, bool firstChance)
    {
        // Page protections are changed before set/delete publishes its metadata.
        // Wait for the transaction before classifying this memory-breakpoint fault.
        std::unique_lock<std::recursive_mutex> lock(mProcess->breakpointMutex);

        /*
        ASSUME:
        exceptionAddress may or may not have been generated by your breakpoints.
        */
        char error[128] = "";
        auto reportError = [&]()
        {
            const bool relock = lock.owns_lock();
            if(relock)
                lock.unlock();
            cbInternalError(error);
            if(relock)
                lock.lock();
        };
        auto exceptionAddress = ptr(exceptionRecord.ExceptionInformation[1]);

        //check if the exception address is directly in the range of a memory breakpoint
        auto foundRange = mProcess->memoryBreakpointRanges.find(Range(exceptionAddress, exceptionAddress));
        if(foundRange == mProcess->memoryBreakpointRanges.end())
        {
            //if not in range, check if a memory breakpoint is in the accessed page
            auto foundPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
            if(foundPage != mProcess->memoryBreakpointPages.end())
            {
                //(this means that by our fault the program generated an exception, we should clean it)
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;

                //We restore the protection
                if(!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", (void*)foundPage->first);
                    reportError();
                }

                //However the following situations may occur:
                // The instruction we singlestep to is a software breakpoint, which may execute a callback, that can :
                // -actually delete a memory breakpoint that takes this page into account
                // -add more memory breakpoints
                //The solution: We just try to see if the page is mapped into memoryBreakpointPages. If the page is in deed being used by any memory breakpoint,
                // then we ought to restore the protection.
                mProcess->StepInternal([this, pBaseAddr]()
                {
                    std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

                    //seek out the page address
                    auto found_page = mProcess->memoryBreakpointPages.find(pBaseAddr);
                    if(found_page == mProcess->memoryBreakpointPages.end())
                    {
                        //no page being used by bpx? Then just return
                        return;
                    }
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, found_page->second.NewProtect);
                    return;
                });
            }
            return;
        }

        /*
        ASSUME:
        exceptionAddress is indeed inside a breakpoint range you have defined.
        */
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if(foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", (void*)exceptionAddress);
            reportError();
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED:
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        if(bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", (void*)(exceptionAddress & ~(PAGE_SIZE - 1)));
            reportError();
            return;
        }
        auto pageAddr = bpxPage->first;
        auto pageProperties = bpxPage->second;

        // PAGE_GUARD is shared by every breakpoint range on this page. Match against the
        // range that contains the accessed byte; the aggregate page type may include a
        // different range and must only be used to derive the page protection.
        const auto accessType = exceptionRecord.ExceptionInformation[0];
        const auto breakpointType = info.internal.memory.type;
        const auto isAccessBreakpoint = breakpointType == MemoryType::Access;
        const auto matchesRead = accessType == 0 && (breakpointType == MemoryType::Read || isAccessBreakpoint);
        const auto matchesWrite = accessType == 1 && (breakpointType == MemoryType::Write || isAccessBreakpoint);
        const auto matchesExecute = accessType == 8 && (breakpointType == MemoryType::Execute || isAccessBreakpoint);
        if(!matchesRead && !matchesWrite && !matchesExecute)
        {
            mContinueStatus = DBG_CONTINUE;
            // This page belongs to some memory breakpoint, but not one that should trigger on this access type.
            // Restore the original protection, single-step the faulting instruction, then re-apply the guard/page
            // permissions if the page is still tracked by any memory breakpoint.
            if(!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
            {
                sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
                reportError();
            }

            mProcess->StepInternal([this, pageAddr]()
            {
                std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

                auto found_page = mProcess->memoryBreakpointPages.find(pageAddr);
                if(found_page == mProcess->memoryBreakpointPages.end())
                    return;
                mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
                return;
            });
            return;
        }


        /*
        ASSUME:
        The breakpoint at exceptionAddress was indeed generated by me.
        Its safe to call the callbacks.
        */
        // Copy the callback while the maps are locked, then release the lock before
        // notifying x64dbg. Breakpoint callbacks can synchronously add or delete
        // breakpoints and must not block behind the transaction lock we hold here.
        BreakpointCallback breakpointCallback;
        const auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if(bpxCb != mProcess->breakpointCallbacks.end())
            breakpointCallback = bpxCb->second;
        lock.unlock();

        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED:
        if(breakpointCallback)
            breakpointCallback(info);


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if(!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
            reportError();
        }
        //Pass info as well
        mProcess->StepInternal([this, pageAddr]()
        {
            std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

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
            if(found_page != mProcess->memoryBreakpointPages.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
            }
            return;
        });

        if(info.singleshoot)
        {
            mProcess->DeleteMemoryBreakpoint(exceptionAddress);
        }

    }


    void Debugger::exceptionAccessViolation(const EXCEPTION_RECORD & exceptionRecord, bool firstChance)
    {
        std::unique_lock<std::recursive_mutex> lock(mProcess->breakpointMutex);

        /*
        ASSUME:
        exceptionAddress may or may not have been generated by your breakpoints.
        */
        char error[128] = "";
        auto reportError = [&]()
        {
            const bool relock = lock.owns_lock();
            if(relock)
                lock.unlock();
            cbInternalError(error);
            if(relock)
                lock.lock();
        };
        auto exceptionAddress = ptr(exceptionRecord.ExceptionInformation[1]);

        //check if the exception address is directly in the range of a memory breakpoint
        auto foundRange = mProcess->memoryBreakpointRanges.find(Range(exceptionAddress, exceptionAddress));
        if(foundRange == mProcess->memoryBreakpointRanges.end())
        {
            //if not in range, check if a memory breakpoint is in the accessed page
            auto foundPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
            if(foundPage != mProcess->memoryBreakpointPages.end())
            {
                //(this means that by our fault the program generated an exception, we should clean it)
                mContinueStatus = DBG_CONTINUE;
                //if the page contains a memory breakpoint we have to restore the old protection to correctly resume the debuggee
                const auto & page = foundPage->second;
                const auto pBaseAddr = foundPage->first;

                //We restore the protection
                if(!mProcess->MemProtect(foundPage->first, PAGE_SIZE, foundPage->second.OldProtect))
                {
                    sprintf_s(error, "MemProtect failed on 0x%p", (void*)foundPage->first);
                    reportError();
                }

                //However the following situations may occur:
                // The instruction we singlestep to is a software breakpoint, which may execute a callback, that can :
                // -actually delete a memory breakpoint that takes this page into account
                // -add more memory breakpoints
                //The solution: We just try to see if the page is mapped into memoryBreakpointPages. If the page is in deed being used by any memory breakpoint,
                // then we ought to restore the protection.
                mProcess->StepInternal([this, pBaseAddr]()
                {
                    std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

                    //seek out the page address
                    auto found_page = mProcess->memoryBreakpointPages.find(pBaseAddr);
                    if(found_page == mProcess->memoryBreakpointPages.end())
                    {
                        //no page being used by bpx? Then just return
                        return;
                    }
                    mProcess->MemProtect(pBaseAddr, PAGE_SIZE, found_page->second.NewProtect);
                    return;
                });
            }
            return;
        }

        /*
        ASSUME:
        exceptionAddress is indeed inside a breakpoint range you have defined.
        */
        auto foundInfo = mProcess->breakpoints.find({ BreakpointType::Memory, foundRange->first });
        if(foundInfo == mProcess->breakpoints.end())
        {
            sprintf_s(error, "inconsistent memory breakpoint at 0x%p", (void*)exceptionAddress);
            reportError();
            return;
        }

        //check if the memory breakpoint is disabled (meaning we shouldn't intercept the exception)
        //TODO: think about what happens with multiple breakpoints in one page where only one is disabled
        //There is really no problem about this because enabled is a property of a range and ranges do not overlap.
        const auto info = foundInfo->second;

        //TODO: check if the right type is accessed (ExceptionInformation[0])
        //FIXED:
        auto bpxPage = mProcess->memoryBreakpointPages.find(exceptionAddress & ~(PAGE_SIZE - 1));
        if(bpxPage == mProcess->memoryBreakpointPages.end())
        {
            sprintf_s(error, "Process::memoryBreakPointPages data structure is incosistent, should dump page at 0x%p", (void*)(exceptionAddress & ~(PAGE_SIZE - 1)));
            reportError();
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
        // Access violations generated by memory breakpoints are write or execute faults.
        // First verify that the aggregate page state could have caused this fault. A
        // debuggee-generated violation must still be passed to the debuggee.
        const auto accessType = exceptionRecord.ExceptionInformation[0];
        const bool pageBreakpointCausedFault =
            (accessType == 1 && pageProperties.WriteRefs != 0) ||
            (accessType == 8 && pageProperties.ExecuteRefs != 0);
        if(!pageBreakpointCausedFault)
            return;

        // Several byte-disjoint ranges can share this page. Only invoke the callback
        // when the range containing the accessed byte matches the actual access type.
        const auto breakpointType = info.internal.memory.type;
        const bool isAccessBreakpoint = breakpointType == MemoryType::Access;
        const bool matchesBreakpoint =
            (accessType == 1 && (breakpointType == MemoryType::Write || isAccessBreakpoint)) ||
            (accessType == 8 && (breakpointType == MemoryType::Execute || isAccessBreakpoint));
        if(!matchesBreakpoint)
        {
            mContinueStatus = DBG_CONTINUE;
            if(!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
            {
                sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
                reportError();
            }

            // The page-wide protection belongs to another range. Execute this
            // instruction once with the original protection, then re-arm the page if
            // any memory breakpoint still owns it.
            mProcess->StepInternal([this, pageAddr]()
            {
                std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

                auto foundPage = mProcess->memoryBreakpointPages.find(pageAddr);
                if(foundPage != mProcess->memoryBreakpointPages.end())
                    mProcess->MemProtect(pageAddr, PAGE_SIZE, foundPage->second.NewProtect);
            });
            return;
        }
        /*
        ASSUME:
        The breakpoint at exceptionAddress was indeed generated by me.
        */
        BreakpointCallback breakpointCallback;
        const auto bpxCb = mProcess->breakpointCallbacks.find({ BreakpointType::Memory, info.address });
        if(bpxCb != mProcess->breakpointCallbacks.end())
            breakpointCallback = bpxCb->second;
        lock.unlock();

        //generic breakpoint callback function.
        cbBreakpoint(info);

        //TODO: execute the user callback (if present)
        //FIXED:
        if(breakpointCallback)
            breakpointCallback(info);


        mContinueStatus = DBG_CONTINUE;
        //TODO: single step and restore page protection
        //FIXED:
        if(!mProcess->MemProtect(pageAddr, PAGE_SIZE, pageProperties.OldProtect))
        {
            sprintf_s(error, "MemProtect failed on 0x%p", (void*)pageAddr);
            reportError();
        }
        //Pass info as well
        mProcess->StepInternal([this, pageAddr]()
        {
            std::lock_guard<std::recursive_mutex> lock(mProcess->breakpointMutex);

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
            if(found_page != mProcess->memoryBreakpointPages.end())
            {
                mProcess->MemProtect(pageAddr, PAGE_SIZE, found_page->second.NewProtect);
            }
            return;
        });

        if(info.singleshoot)
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
        switch(exceptionInfo.ExceptionRecord.ExceptionCode)
        {
        case STATUS_BREAKPOINT:
        case STATUS_WX86_BREAKPOINT:
            exceptionBreakpoint(exceptionRecord, firstChance);
            break;
        case STATUS_SINGLE_STEP:
        case STATUS_WX86_SINGLE_STEP:
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
        if(mContinueStatus == DBG_EXCEPTION_NOT_HANDLED)
            cbUnhandledException(exceptionRecord, firstChance);
    }
};