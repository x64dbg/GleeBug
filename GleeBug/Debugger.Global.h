#ifndef _DEBUGGER_GLOBAL_H
#define _DEBUGGER_GLOBAL_H

#include "GleeBug.h"

namespace GleeBug
{
    //forward declarations
    class Debugger;
    class ProcessInfo;
    class DllInfo;
    class ThreadInfo;
    enum class BreakpointType;
    struct BreakpointInfo;

    //key typedefs
    typedef std::pair<BreakpointType, ptr> BreakpointKey;

    //callback function typedefs
    typedef std::function<void()> StepCallback;
    typedef std::function<void(const BreakpointInfo & info)> BreakpointCallback;

    //map typedefs
    typedef std::map<uint32, ProcessInfo> ProcessMap;
    typedef std::map<Range, DllInfo, RangeCompare> DllMap;
    typedef std::map<uint32, ThreadInfo> ThreadMap;
    typedef std::map<BreakpointKey, BreakpointInfo> BreakpointMap;
    typedef std::map<BreakpointKey, BreakpointCallback> BreakpointCallbackMap;

    //vector typedefs
    typedef std::vector<StepCallback> StepCallbackVector;
};

#endif //_DEBUGGER_GLOBAL_H