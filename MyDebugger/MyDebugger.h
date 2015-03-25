#ifndef _MYDEBUGGER_H
#define _MYDEBUGGER_H

#include "../GleeBug/Debugger.h"

class MyDebugger : public GleeBug::Debugger
{
protected:
	virtual void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO* createProcess)
	{
		printf("Process %d created with entry 0x%p\n", _debugEvent.dwProcessId, createProcess->lpStartAddress);
	};

	virtual void cbExitProcessEvent(const EXIT_PROCESS_DEBUG_INFO* exitProcess)
	{
		printf("Process %d terminated with exit code 0x%08X\n", _debugEvent.dwProcessId, exitProcess->dwExitCode);
	}

	virtual void cbCreateThreadEvent(const CREATE_THREAD_DEBUG_INFO* createThread)
	{
		printf("Thread %d created with entry 0x%p\n", _debugEvent.dwThreadId, createThread->lpStartAddress);
	};

	virtual void cbException_single_spep(EXCEPTION_RECORD* except_inf) 
	{
		printf("a single step occurred at location 0x%X", except_inf->ExceptionAddress);
	};

	virtual void cbExcpetion_breakpoint(EXCEPTION_RECORD* except_inf) 
	{
		printf("a breakpoint occurred at location 0x%X", except_inf->ExceptionAddress);
	};

	virtual void cbExitThreadEvent(const EXIT_THREAD_DEBUG_INFO* exitThread)
	{
		printf("Thread %d terminated with exit code 0x%08X\n", _debugEvent.dwThreadId, exitThread->dwExitCode);
	};

	virtual void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO* loadDll)
	{
		printf("DLL loaded at 0x%p\n", loadDll->lpBaseOfDll);
	};

	virtual void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO* unloadDll)
	{
		printf("DLL 0x%p unloaded\n", unloadDll->lpBaseOfDll);
	};

	virtual void cbExceptionEvent(const EXCEPTION_DEBUG_INFO* exceptionInfo)
	{
		printf("Exception with code 0x%08X\n", exceptionInfo->ExceptionRecord);
	};

	virtual void cbDebugStringEvent(const OUTPUT_DEBUG_STRING_INFO* debugString)
	{
		printf("Debug string at 0x%p with length %d\n", debugString->lpDebugStringData, debugString->nDebugStringLength);
	};

	virtual void cbRipEvent(const RIP_INFO* rip)
	{
		printf("RIP event type 0x%X, error 0x%X", rip->dwType, rip->dwError);
	};
};

#endif //_MYDEBUGGER_H