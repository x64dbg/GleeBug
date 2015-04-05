#ifndef _DEBUGGER_GLOBAL_H
#define _DEBUGGER_GLOBAL_H

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <functional>

#include <windows.h>
#include <psapi.h>

namespace GleeBug
{
	typedef std::pair<ULONG_PTR, ULONG_PTR> Range;

	struct RangeCompare
	{
		inline bool operator()(const Range & a, const Range & b) const //a before b?
		{
			return a.second < b.first;
		}
	};

	//forward declarations
	class Debugger;
	class ProcessInfo;
	class DllInfo;
	class ThreadInfo;

	//map typedefs
	typedef std::map<DWORD, ProcessInfo> ProcessMap;
	typedef std::map<Range, DllInfo, RangeCompare> DllMap;
	typedef std::map<DWORD, ThreadInfo> ThreadMap;
	
	//callback function typedefs
	typedef std::function<void()> StepCallback;

	//vector typedefs
	typedef std::vector<StepCallback> StepCallbackVector;

	//macros
	#define BIND(thisPtr, funcPtr) std::bind(&funcPtr, thisPtr)
};

#endif //_DEBUGGER_GLOBAL_H