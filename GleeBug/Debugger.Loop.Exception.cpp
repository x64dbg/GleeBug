#include "Debugger.h"

namespace GleeBug
{
    void Debugger::exceptionBreakpoint(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (!_process->systemBreakpoint) //handle system breakpoint
        {
            _process->systemBreakpoint = true;
            _continueStatus = DBG_CONTINUE;

            //call the callback
            cbSystemBreakpoint();
        }
    }

    void Debugger::exceptionSingleStep(const EXCEPTION_RECORD & exceptionRecord, const bool firstChance)
    {
        if (_thread->isSingleStepping) //handle single step
        {
            _thread->isSingleStepping = false;
            _continueStatus = DBG_CONTINUE;

            //call the callbacks
            StepCallbackVector cbStepCopy = _thread->stepCallbacks;
            _thread->stepCallbacks.clear();
            for (auto cbStep : cbStepCopy)
                cbStep();
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
        const bool firstChance = exceptionInfo.dwFirstChance == 1;

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

        //call the debug event callback
        cbExceptionEvent(exceptionInfo);
    }
};