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
            const auto & info = foundInfo->second;

            //set continue status
            _continueStatus = DBG_CONTINUE;

            //set back the instruction pointer
            _registers->Gip = info.address;

            //restore the original breakpoint byte and do an internal step
            _process->MemWrite(info.address, info.internal.software.oldbytes, info.internal.software.size);
            _thread->StepInternal(std::bind([this, info]()
            {
                _process->MemWrite(info.address, info.internal.software.newbytes, info.internal.software.size);
            }));

            //call the generic callback
            cbBreakpoint(info);

            //call the user callback
            auto foundCallback = _process->breakpointCallbacks.find({ BreakpointType::Software, info.address });
            if (foundCallback != _process->breakpointCallbacks.end())
                foundCallback->second(info);
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
        else //handle other single step exceptions
        {
        }
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