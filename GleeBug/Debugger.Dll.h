#ifndef _DEBUGGER_DLL_H
#define _DEBUGGER_DLL_H

#include "Debugger.Global.h"

namespace GleeBug
{
	/**
	\brief DLL information structure.
	*/
	struct DllInfo
	{
		ULONG_PTR lpBaseOfDll;
		DWORD sizeOfImage;
		ULONG_PTR entryPoint;

		DllInfo();
		DllInfo(LPVOID lpBaseOfDll, DWORD sizeOfImage, LPVOID entryPoint);
	};

	typedef std::map<Range, DllInfo, RangeCompare> DllMap;
};

#endif //_DEBUGGER_DLL_H