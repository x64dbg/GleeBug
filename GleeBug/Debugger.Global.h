#ifndef DEBUGGER_GLOBAL_H
#define DEBUGGER_GLOBAL_H

#include "GleeBug.h"
#include <memory>

//defines
#define GLEEBUG_HWBP_COUNT 4
#define GLEEBUG_PAGE_SIZE 0x1000

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif // PAGE_SIZE

#ifndef STATUS_WX86_SINGLE_STEP
#define STATUS_WX86_SINGLE_STEP ((DWORD)0x4000001EL)
#endif // STATUS_WX86_SINGLE_STEP

#ifndef STATUS_WX86_BREAKPOINT
#define STATUS_WX86_BREAKPOINT ((DWORD)0x4000001FL)
#endif // STATUS_WX86_BREAKPOINT

namespace GleeBug
{
    //forward declarations
    class Debugger;
    class Process;
    class Dll;
    class Thread;
    enum class BreakpointType;
    struct BreakpointInfo;
    struct MemoryBreakpointData;

    const int HWBP_COUNT = GLEEBUG_HWBP_COUNT;

    //key typedefs
    typedef std::pair<BreakpointType, ptr> BreakpointKey;

    //callback function typedefs
    typedef std::function<void()> StepCallback;
    typedef std::function<void(const BreakpointInfo & info)> BreakpointCallback;

    //map typedefs
    typedef std::map<uint32, std::unique_ptr<Process>> ProcessMap;
    typedef std::map<uint32, std::unique_ptr<Thread>> ThreadMap;
    typedef std::map<BreakpointKey, BreakpointInfo> BreakpointMap;
    typedef std::map<BreakpointKey, BreakpointCallback> BreakpointCallbackMap;
    typedef std::unordered_map<ptr, BreakpointMap::iterator> SoftwareBreakpointMap;
    typedef std::set<Range, RangeCompare> MemoryBreakpointSet;
    typedef std::unordered_map<ptr, MemoryBreakpointData> MemoryBreakpointMap;

    //vector typedefs
    typedef std::vector<StepCallback> StepCallbackVector;
};

#endif //DEBUGGER_GLOBAL_H