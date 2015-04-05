#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread)
	{
		//thread housekeeping
		_process->threads.insert({ _debugEvent.dwThreadId,
			ThreadInfo(_debugEvent.dwThreadId, createThread.hThread, createThread.lpThreadLocalBase, createThread.lpStartAddress) });

		//set the current thread
		_thread = _process->thread = &_process->threads.find(_debugEvent.dwThreadId)->second;
		if (!_thread->RegReadContext())
			cbInternalError("ThreadInfo::RegReadContext() failed!");

		//call the debug event callback
		cbCreateThreadEvent(createThread, *_thread);
	}

	void Debugger::exitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread)
	{
		//call the debug event callback
		cbExitThreadEvent(exitThread, *_thread);

		//thread housekeeping
		_process->threads.erase(_debugEvent.dwThreadId);

		//set the current thread
		_thread = _process->thread = nullptr;
	}
};