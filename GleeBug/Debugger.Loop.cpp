#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
	{
		//process housekeeping
		ProcessInfo process(createProcess.hProcess,
			createProcess.hThread,
			_debugEvent.dwProcessId,
			_debugEvent.dwThreadId);
		_processes.insert({ process.dwProcessId, process });

		//set the current process and current thread
		_curProcess = &_processes[process.dwProcessId];
		_curProcess->curThread = &_curProcess->threads[process.dwMainThreadId];

		//call the callback
		cbCreateProcessEvent(createProcess, *_curProcess);

		//close the file handle
		CloseHandle(createProcess.hFile);
	}

	void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
	{
		//check if the terminated process is the main debuggee
		if (_debugEvent.dwProcessId == _mainProcess.dwProcessId)
			_breakDebugger = true;

		//call the callback
		cbExitProcessEvent(exitProcess, *_curProcess);

		//process housekeeping
		_processes.erase(_debugEvent.dwProcessId);

		//set the current process
		_curProcess = nullptr;
	}

	void Debugger::createThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread)
	{
		//thread housekeeping
		ThreadInfo thread(_debugEvent.dwThreadId, createThread.hThread, createThread.lpThreadLocalBase, createThread.lpStartAddress);
		_curProcess->threads.insert({ thread.dwThreadId, thread });

		//set the current thread
		_curProcess->curThread = &_curProcess->threads[thread.dwThreadId];

		//call the callback
		cbCreateThreadEvent(createThread, *_curProcess->curThread);
	}

	void Debugger::exitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread)
	{
		//call the callback
		cbExitThreadEvent(exitThread, *_curProcess->curThread);

		//thread housekeeping
		_curProcess->threads.erase(_debugEvent.dwThreadId);

		//set the current thread
		_curProcess->curThread = nullptr;
	}

	void Debugger::loadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll)
	{
		//DLL housekeeping
		MODULEINFO modinfo;
		memset(&modinfo, 0, sizeof(MODULEINFO));
		GetModuleInformation(_curProcess->hProcess,
			(HMODULE)loadDll.lpBaseOfDll,
			&modinfo,
			sizeof(MODULEINFO));
		DllInfo dll(loadDll.lpBaseOfDll, modinfo.SizeOfImage, modinfo.EntryPoint);
		_curProcess->dlls.insert({ Range(dll.lpBaseOfDll, dll.lpBaseOfDll + dll.sizeOfImage - 1), dll });

		//call the callback
		cbLoadDllEvent(loadDll, dll);

		//close the file handle
		CloseHandle(loadDll.hFile);
	}

	void Debugger::unloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll)
	{
		//call the callback
		ULONG_PTR lpBaseOfDll = (ULONG_PTR)unloadDll.lpBaseOfDll;
		auto dll = _curProcess->dlls.find(Range(lpBaseOfDll, lpBaseOfDll));
		if (dll != _curProcess->dlls.end())
			cbUnloadDllEvent(unloadDll, dll->second);
		else
			cbUnloadDllEvent(unloadDll, DllInfo(unloadDll.lpBaseOfDll, 0, 0));

		//DLL housekeeping
		if (dll != _curProcess->dlls.end())
			_curProcess->dlls.erase(dll);
	}

	void Debugger::exceptionEvent(const EXCEPTION_DEBUG_INFO & exceptionInfo)
	{
		cbExceptionEvent(exceptionInfo);
	}

	void Debugger::debugStringEvent(const OUTPUT_DEBUG_STRING_INFO & debugString)
	{
		cbDebugStringEvent(debugString);
	}

	void Debugger::ripEvent(const RIP_INFO & rip)
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

			//set the current process and thread
			if (_processes.count(_debugEvent.dwProcessId))
			{
				_curProcess = &_processes[_debugEvent.dwProcessId];
				if (_curProcess->threads.count(_debugEvent.dwThreadId))
					_curProcess->curThread = &_curProcess->threads[_debugEvent.dwThreadId];
				else
					_curProcess->curThread = nullptr;
			}
			else
				_curProcess = nullptr;

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

			if (!ContinueDebugEvent(_debugEvent.dwProcessId, _debugEvent.dwThreadId, _continueStatus))
				break;
		}

		_processes.clear();
		_curProcess = nullptr;
	}
};