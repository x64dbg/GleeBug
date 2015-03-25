#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createProcessEvent(CREATE_PROCESS_DEBUG_INFO* CreateProcess)
	{
		log("Debugger::createProcessEvent");
	}

	void Debugger::exitProcessEvent(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
	{
		log("Debugger::exitProcessEvent");
		if (_debugEvent.dwProcessId == _mainProcess.ProcessId)
		{
			_breakDebugger = true;
		}
	}

	void Debugger::createThreadEvent(CREATE_THREAD_DEBUG_INFO* CreateThread)
	{
		log("Debugger::createThreadEvent");
	}

	void Debugger::exitThreadEvent(EXIT_THREAD_DEBUG_INFO* ExitThread)
	{
		log("Debugger::exitThreadEvent");
	}

	void Debugger::loadDllEvent(LOAD_DLL_DEBUG_INFO* LoadDll)
	{
		log("Debugger::loadDllEvent");
	}

	void Debugger::unloadDllEvent(UNLOAD_DLL_DEBUG_INFO* UnloadDll)
	{
		log("Debugger::unloadDllEvent");
	}

	void Debugger::exceptionEvent(EXCEPTION_DEBUG_INFO* Exception)
	{
		log("Debugger::exceptionEvent");
	}

	void Debugger::debugStringEvent(OUTPUT_DEBUG_STRING_INFO* DebugString)
	{
		log("Debugger::debugStringEvent");
	}

	void Debugger::ripEvent(RIP_INFO* Rip)
	{
		log("Debugger::ripEvent");
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