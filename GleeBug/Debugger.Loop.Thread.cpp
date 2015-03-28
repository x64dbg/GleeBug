#include "Debugger.h"

namespace GleeBug
{
	void Debugger::createThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread)
	{
		//thread housekeeping
		ThreadInfo thread(_debugEvent.dwThreadId, createThread.lpThreadLocalBase, createThread.lpStartAddress);
		_curProcess->threads.insert({ thread.dwThreadId, thread });

		//set the current thread
		_curProcess->curThread = &_curProcess->threads[thread.dwThreadId];

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