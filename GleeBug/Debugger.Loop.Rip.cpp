#include "Debugger.h"

namespace GleeBug
{
    void Debugger::ripEvent(const RIP_INFO & rip)
    {
        //prevent anti-debug trick (RIP events are actually exceptions)
        _continueStatus = DBG_EXCEPTION_NOT_HANDLED;

        //call the debug event callback
        cbRipEvent(rip);
    }
};