#include "Debugger.h"

namespace GleeBug
{
    void Debugger::debugStringEvent(const OUTPUT_DEBUG_STRING_INFO & debugString)
    {
        //prevent anti-debug trick (debug string events are actually exceptions)
        mContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

        //call the debug event callback
        cbDebugStringEvent(debugString);
    }
};