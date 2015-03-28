#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
	{
		//process housekeeping
		ProcessInfo process(_debugEvent.dwProcessId,
			_debugEvent.dwThreadId);
		_processes.insert({ process.dwProcessId, process });

		//set the current process and current thread
		_curProcess = &_processes[process.dwProcessId];
		_curProcess->curThread = &_curProcess->threads[process.dwMainThreadId];

		//call the debug event callback
		cbCreateProcessEvent(createProcess, *_curProcess);

		//close the file handle
		CloseHandle(createProcess.hFile);
	}

	void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
	{
		//check if the terminated process is the main debuggee
		if (_debugEvent.dwProcessId == _mainProcess.dwProcessId)
			_breakDebugger = true;

		//call the debug event callback
		cbExitProcessEvent(exitProcess, *_curProcess);

		//process housekeeping
		_processes.erase(_debugEvent.dwProcessId);

		//set the current process
		_curProcess = nullptr;
	}
};