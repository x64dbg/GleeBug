#include "Debugger.Thread.h"

namespace GleeBug
{
	ThreadInfo::ThreadInfo()
	{
		this->hThread = INVALID_HANDLE_VALUE;
	}

	ThreadInfo::ThreadInfo(DWORD dwThreadId, HANDLE hThread, LPVOID lpThreadLocalBase, LPVOID lpStartAddress)
	{
		this->dwThreadId = dwThreadId;
		this->hThread = hThread;
		this->lpThreadLocalBase = (ULONG_PTR)lpThreadLocalBase;
		this->lpStartAddress = (ULONG_PTR)lpStartAddress;
	}

	bool ThreadInfo::RegReadContext()
	{
		SuspendThread(this->hThread);
		memset(&this->_oldContext, 0, sizeof(CONTEXT));
		this->_oldContext.ContextFlags = CONTEXT_ALL;
		bool bReturn = false;
		if (GetThreadContext(this->hThread, &this->_oldContext))
		{
			this->registers.SetContext(this->_oldContext);
			bReturn = true;
		}
		ResumeThread(this->hThread);
		return bReturn;
	}

	bool ThreadInfo::RegWriteContext()
	{
		//check if something actually changed
		if (memcmp(&this->_oldContext, this->registers.GetContext(), sizeof(CONTEXT)) == 0)
			return true;
		//update the context
		SuspendThread(this->hThread);
		bool bReturn = !!SetThreadContext(this->hThread, this->registers.GetContext());
		ResumeThread(this->hThread);
		return bReturn;
	}

	void ThreadInfo::StepInto(StepCallback cbStep)
	{
		StepInto();
		stepCallbacks.push_back(cbStep);
	}

	void ThreadInfo::StepInto()
	{
		registers.SetTrapFlag();
		isSingleStepping = true;
	}
};