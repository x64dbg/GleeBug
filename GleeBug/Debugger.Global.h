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

    //map typedefs
    typedef std::map<uint32, ProcessInfo> ProcessMap;
    typedef std::map<Range, DllInfo, RangeCompare> DllMap;
    typedef std::map<uint32, ThreadInfo> ThreadMap;

    //callback function typedefs
    typedef std::function<void()> StepCallback;

    //vector typedefs
    typedef std::vector<StepCallback> StepCallbackVector;
};

#endif //_DEBUGGER_GLOBAL_H