#include "Debugger.h"

namespace GleeBug
{
    void Debugger::unknownEvent(DWORD debugEventCode)
    {
        //prevent possible anti-debug trick
        _continueStatus = DBG_EXCEPTION_NOT_HANDLED;

        //call the debug event callback
        cbUnknownEvent(debugEventCode);
    }
};