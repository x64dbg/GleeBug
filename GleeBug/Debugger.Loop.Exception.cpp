#include "Debugger.h"

namespace GleeBug
{
    void Debugger::exceptionBreakpoint(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (!_process->systemBreakpoint) //handle system breakpoint
        {
            //set internal state
            _process->systemBreakpoint = true;
            _continueStatus = DBG_CONTINUE;

            //call the callback
            cbSystemBreakpoint();
        }
        else
        {
            //check if the breakpoint exists
            auto foundInfo = _process->breakpoints.find({ BreakpointType::Software, ptr(exceptionRecord.ExceptionAddress) });
            if (foundInfo == _process->breakpoints.end())
                return;
            const auto info = foundInfo->second;

            //set continue status
            _continueStatus = DBG_CONTINUE;

            //set back the instruction pointer
            _registers->Gip = info.address;

            //restore the original breakpoint byte and do an internal step
            _process->MemWrite(info.address, info.internal.software.oldbytes, info.internal.software.size);
            _thread->StepInternal(std::bind([this, info]()
            {
                //only restore the bytes if the breakpoint still exists
                if (_process->breakpoints.find({ BreakpointType::Software, info.address }) != _process->breakpoints.end())
                    _process->MemWrite(info.address, info.internal.software.newbytes, info.internal.software.size);
            }));

            //call the generic callback
            cbBreakpoint(info);

            //call the user callback
            auto foundCallback = _process->breakpointCallbacks.find({ BreakpointType::Software, info.address });
            if (foundCallback != _process->breakpointCallbacks.end())
                foundCallback->second(info);

            //delete the breakpoint if it is singleshoot
            if (info.singleshoot)
                _process->DeleteGenericBreakpoint(info);
        }
    }

    void Debugger::exceptionSingleStep(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (_thread->isInternalStepping) //handle internal steps
        {
            //set internal status
            _thread->isInternalStepping = false;
            _continueStatus = DBG_CONTINUE;

            //call the internal step callback
            _thread->cbInternalStep();
        }
        if (_thread->isSingleStepping) //handle single step
        {
            //set internal status
            _thread->isSingleStepping = false;
            _continueStatus = DBG_CONTINUE;

            //call the generic callback
            cbStep();

            //call the user callbacks
            auto cbStepCopy = _thread->stepCallbacks;
            _thread->stepCallbacks.clear();
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
        ptr dr6 = _registers->Dr6();
        HardwareSlot breakpointSlot;
        ptr breakpointAddress;
        if (exceptionAddress == _registers->Dr0() || dr6 & 0x1)
        {
            breakpointAddress = _registers->Dr0();
            breakpointSlot = HardwareSlot::Dr0;
        }
        else if (exceptionAddress == _registers->Dr1() || dr6 & 0x2)
        {
            breakpointAddress = _registers->Dr1();
            breakpointSlot = HardwareSlot::Dr1;
        }
        else if (exceptionAddress == _registers->Dr2() || dr6 & 0x4)
        {
            breakpointAddress = _registers->Dr2();
            breakpointSlot = HardwareSlot::Dr2;
        }
        else if (exceptionAddress == _registers->Dr3() || dr6 & 0x8)
        {
            breakpointAddress = _registers->Dr3();
            breakpointSlot = HardwareSlot::Dr3;
        }
        else
            return; //not a hardware breakpoint

        //find the breakpoint in the internal structures
        auto foundInfo = _process->breakpoints.find({ BreakpointType::Hardware, breakpointAddress });
        if (foundInfo == _process->breakpoints.end())
            return; //not a valid hardware breakpoint
        const auto info = foundInfo->second;
        if (info.internal.hardware.slot != breakpointSlot)
            return; //not a valid hardware breakpoint

        //set continue status
        _continueStatus = DBG_CONTINUE;

        //delete the hardware breakpoint from the thread (not the breakpoint buffer) and do an internal step (TODO: maybe delete from all threads?)
        _thread->DeleteHardwareBreakpoint(breakpointSlot);
        _thread->StepInternal(std::bind([this, info]()
        {
            //only restore if the breakpoint still exists
            if (_process->breakpoints.find({ BreakpointType::Hardware, info.address }) != _process->breakpoints.end())
                _thread->SetHardwareBreakpoint(info.address, info.internal.hardware.slot, info.internal.hardware.type, info.internal.hardware.size);
        }));

        //call the generic callback
        cbBreakpoint(info);

        //call the user callback
        auto foundCallback = _process->breakpointCallbacks.find({ BreakpointType::Hardware, info.address });
        if (foundCallback != _process->breakpointCallbacks.end())
            foundCallback->second(info);

        //delete the breakpoint if it is singleshoot
        if (info.singleshoot)
            _process->DeleteGenericBreakpoint(info);
    }

    void Debugger::exceptionEvent(const EXCEPTION_DEBUG_INFO & exceptionInfo)
    {
        //let the debuggee handle exceptions per default
        _continueStatus = DBG_EXCEPTION_NOT_HANDLED;

        const EXCEPTION_RECORD & exceptionRecord = exceptionInfo.ExceptionRecord;
        bool firstChance = exceptionInfo.dwFirstChance == 1;

        //call the debug event callback
        cbExceptionEvent(exceptionInfo);

        //dispatch the exception
        switch (exceptionInfo.ExceptionRecord.ExceptionCode)
        {
        case STATUS_BREAKPOINT:
            exceptionBreakpoint(exceptionRecord, firstChance);
            break;
        case STATUS_SINGLE_STEP:
            exceptionSingleStep(exceptionRecord, firstChance);
            break;
        }

        //call the unhandled exception callback
        if (_continueStatus == DBG_EXCEPTION_NOT_HANDLED)
            cbUnhandledException(exceptionRecord, firstChance);
    }
};