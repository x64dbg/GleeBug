#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread)
	{
		//thread housekeeping
		_curProcess->threads.insert({ _debugEvent.dwThreadId,
			ThreadInfo(_debugEvent.dwThreadId, createThread.hThread, createThread.lpThreadLocalBase, createThread.lpStartAddress) });

		//set the current thread
		_curProcess->curThread = &_curProcess->threads.find(_debugEvent.dwThreadId)->second;
		if (!_curProcess->curThread->RegReadContext())
			cbInternalError("ThreadInfo::RegReadContext() failed!");

		//call the debug event callback
		cbCreateThreadEvent(createThread, *_curProcess->curThread);
	}

	void Debugger::exitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread)
	{
		//call the debug event callback
		cbExitThreadEvent(exitThread, *_curProcess->curThread);

		//thread housekeeping
		_curProcess->threads.erase(_debugEvent.dwThreadId);

		//set the current thread
		_curProcess->curThread = nullptr;
	}
};