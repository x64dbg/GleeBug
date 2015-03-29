#ifndef _DEBUGGER_GLOBAL_H
#define _DEBUGGER_GLOBAL_H

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include "Debugger.Breakpoint.Types.h"

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
};

#endif //_DEBUGGER_GLOBAL_H