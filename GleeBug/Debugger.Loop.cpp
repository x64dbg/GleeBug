#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createProcessEvent(CREATE_PROCESS_DEBUG_INFO* createProcess)
	{
		cbCreateProcessEvent(createProcess);
	}

	void Debugger::exitProcessEvent(EXIT_PROCESS_DEBUG_INFO* exitProcess)
	{
		if (_debugEvent.dwProcessId == _mainProcess.ProcessId)
		{
			_breakDebugger = true;
		}
		cbExitProcessEvent(exitProcess);
	}

	void Debugger::createThreadEvent(CREATE_THREAD_DEBUG_INFO* createThread)
	{
		cbCreateThreadEvent(createThread);
	}

	void Debugger::exitThreadEvent(EXIT_THREAD_DEBUG_INFO* exitThread)
	{
		cbExitThreadEvent(exitThread);
	}

	void Debugger::loadDllEvent(LOAD_DLL_DEBUG_INFO* loadDll)
	{
		cbLoadDllEvent(loadDll);
	}

	void Debugger::unloadDllEvent(UNLOAD_DLL_DEBUG_INFO* unloadDll)
	{
		cbUnloadDllEvent(unloadDll);
	}

	void Debugger::exceptionEvent(EXCEPTION_DEBUG_INFO* exceptionInfo)
	{
		cbExceptionEvent(exceptionInfo);
	}

	void Debugger::debugStringEvent(OUTPUT_DEBUG_STRING_INFO* debugString)
	{
		cbDebugStringEvent(debugString);
	}

	void Debugger::ripEvent(RIP_INFO* rip)
	{
		cbRipEvent(rip);
	}

	void Debugger::Start()
	{
		_continueStatus = DBG_EXCEPTION_NOT_HANDLED;
		_breakDebugger = false;
		while (!_breakDebugger)
		{
			if (!WaitForDebugEvent(&_debugEvent, INFINITE))
				break;

			switch (_debugEvent.dwDebugEventCode)
			{
			case CREATE_PROCESS_DEBUG_EVENT:
				createProcessEvent(&_debugEvent.u.CreateProcessInfo);
				break;
			case EXIT_PROCESS_DEBUG_EVENT:
				exitProcessEvent(&_debugEvent.u.ExitProcess);
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				createThreadEvent(&_debugEvent.u.CreateThread);
				break;
			case EXIT_THREAD_DEBUG_EVENT:
				exitThreadEvent(&_debugEvent.u.ExitThread);
				break;
			case LOAD_DLL_DEBUG_EVENT:
				loadDllEvent(&_debugEvent.u.LoadDll);
				break;
			case UNLOAD_DLL_DEBUG_EVENT:
				unloadDllEvent(&_debugEvent.u.UnloadDll);
				break;
			case EXCEPTION_DEBUG_EVENT:
				exceptionEvent(&_debugEvent.u.Exception);
				break;
			case OUTPUT_DEBUG_STRING_EVENT:
				debugStringEvent(&_debugEvent.u.DebugString);
				break;
			case RIP_EVENT:
				ripEvent(&_debugEvent.u.RipInfo);
				break;
			}

			if (!ContinueDebugEvent(_debugEvent.dwProcessId, _debugEvent.dwThreadId, _continueStatus))
				break;
		}
	}
};