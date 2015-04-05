#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
	{
		//process housekeeping
		ProcessInfo process(_debugEvent.dwProcessId,
			createProcess.hProcess,
			_debugEvent.dwThreadId);
		_processes.insert({ process.dwProcessId, process });
		_process = &_processes.find(process.dwProcessId)->second;

		//thread housekeeping (main thread is created implicitly)
		ThreadInfo thread(_debugEvent.dwThreadId,
			createProcess.hThread,
			createProcess.lpThreadLocalBase,
			createProcess.lpStartAddress);
		_process->threads.insert({ thread.dwThreadId, thread });
		_thread = _process->thread = &_process->threads.find(thread.dwThreadId)->second;

		//read thread context from main thread
		if (!_thread->RegReadContext())
			cbInternalError("ThreadInfo::RegReadContext() failed!");

		//call the debug event callback
		cbCreateProcessEvent(createProcess, *_process);

		//close the file handle
		CloseHandle(createProcess.hFile);
	}

	void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
	{
		//check if the terminated process is the main debuggee
		if (_debugEvent.dwProcessId == _mainProcess.dwProcessId)
			_breakDebugger = true;

		//call the debug event callback
		cbExitProcessEvent(exitProcess, *_process);

		//process housekeeping
		_processes.erase(_debugEvent.dwProcessId);

		//set the current process
		_process = nullptr;
	}
};