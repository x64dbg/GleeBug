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
		_curProcess = &_processes.find(process.dwProcessId)->second;

		//thread housekeeping (main thread is created implicitly)
		ThreadInfo thread(_debugEvent.dwThreadId,
			createProcess.hThread,
			createProcess.lpThreadLocalBase,
			createProcess.lpStartAddress);
		_curProcess->threads.insert({ thread.dwThreadId, thread });
		_curProcess->curThread = &_curProcess->threads.find(thread.dwThreadId)->second;

		//read thread context from main thread
		if (!_curProcess->curThread->RegReadContext())
			cbInternalError("ThreadInfo::RegReadContext() failed!");

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