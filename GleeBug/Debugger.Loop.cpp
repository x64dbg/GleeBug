#include "Debugger.h"

namespace GleeBug
{
	void Debugger::Start()
	{
		//initialize loop variables
		_breakDebugger = false;

		while (!_breakDebugger)
		{
			//wait for a debug event
			_isRunning = true;
			if (!WaitForDebugEvent(&_debugEvent, INFINITE))
				break;
			_isRunning = false;

			//set default continue status
			_continueStatus = DBG_EXCEPTION_NOT_HANDLED;

			//set the current process and thread
			if (_processes.count(_debugEvent.dwProcessId))
			{
				_process = &_processes[_debugEvent.dwProcessId];
				if (_process->threads.count(_debugEvent.dwThreadId))
				{
					_thread = _process->thread = &_process->threads[_debugEvent.dwThreadId];
					if (!_thread->RegReadContext())
						cbInternalError("ThreadInfo::RegReadContext() failed!");
				}
				else
					_thread = _process->thread = nullptr;
			}
			else
				_process = nullptr;

			//dispatch the debug event
			switch (_debugEvent.dwDebugEventCode)
			{
			case CREATE_PROCESS_DEBUG_EVENT:
				createProcessEvent(_debugEvent.u.CreateProcessInfo);
				break;
			case EXIT_PROCESS_DEBUG_EVENT:
				exitProcessEvent(_debugEvent.u.ExitProcess);
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				createThreadEvent(_debugEvent.u.CreateThread);
				break;
			case EXIT_THREAD_DEBUG_EVENT:
				exitThreadEvent(_debugEvent.u.ExitThread);
				break;
			case LOAD_DLL_DEBUG_EVENT:
				loadDllEvent(_debugEvent.u.LoadDll);
				break;
			case UNLOAD_DLL_DEBUG_EVENT:
				unloadDllEvent(_debugEvent.u.UnloadDll);
				break;
			case EXCEPTION_DEBUG_EVENT:
				exceptionEvent(_debugEvent.u.Exception);
				break;
			case OUTPUT_DEBUG_STRING_EVENT:
				debugStringEvent(_debugEvent.u.DebugString);
				break;
			case RIP_EVENT:
				ripEvent(_debugEvent.u.RipInfo);
				break;
			}

			//write the register context
			if (_thread)
			{
				if (!_thread->RegWriteContext())
					cbInternalError("ThreadInfo::RegWriteContext() failed!");
			}

			//continue the debug event
			if (!ContinueDebugEvent(_debugEvent.dwProcessId, _debugEvent.dwThreadId, _continueStatus))
				break;
		}

		//cleanup
		_processes.clear();
		_process = nullptr;
	}
};