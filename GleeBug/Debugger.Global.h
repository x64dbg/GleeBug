#ifndef DEBUGGER_GLOBAL_H
#define DEBUGGER_GLOBAL_H

#include "GleeBug.h"

//defines
#define GLEEBUG_HWBP_COUNT 4
#define GLEEBUG_PAGE_SIZE 0x1000

namespace GleeBug
{
    //forward declarations
    class Debugger;
    class Process;
    class Dll;
    class Thread;
    enum class BreakpointType;
    struct BreakpointInfo;

    //constants
    const int HWBP_COUNT = GLEEBUG_HWBP_COUNT;
    const int PAGE_SIZE = GLEEBUG_PAGE_SIZE;

    //key typedefs
    typedef std::pair<BreakpointType, ptr> BreakpointKey;

    //callback function typedefs
    typedef std::function<void()> StepCallback;
    typedef std::function<void(const BreakpointInfo & info)> BreakpointCallback;

    //map typedefs
    typedef std::map<uint32, Process> ProcessMap;
    typedef std::map<Range, Dll, RangeCompare> DllMap;
    typedef std::map<uint32, Thread> ThreadMap;
    typedef std::map<BreakpointKey, BreakpointInfo> BreakpointMap;
    typedef std::map<BreakpointKey, BreakpointCallback> BreakpointCallbackMap;
    typedef std::unordered_map<ptr, BreakpointMap::iterator> SoftwareBreakpointMap;
    typedef std::set<Range, RangeCompare> MemoryBreakpointSet;

    //vector typedefs
    typedef std::vector<StepCallback> StepCallbackVector;
};

#endif //DEBUGGER_GLOBAL_H