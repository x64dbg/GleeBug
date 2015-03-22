#ifndef _DEBUGGER_CORE_H
#define _DEBUGGER_CORE_H

#include "_global.h"
#include "Debugger.State.h"

namespace Debugger
{
	bool Init(const wchar_t* szFilePath,
		const wchar_t* szCommandLine,
		const wchar_t* szCurrentDirectory,
		ProcessInfo* process);
	bool Stop();
	bool Detach();
};

#endif //_DEBUGGER_CORE_H