#ifndef _DEBUGGER_THREAD_H
#define _DEBUGGER_THREAD_H

#include "Debugger.Global.h"
#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
	/**
	\brief Thread information structure.
	*/
	class ThreadInfo
	{
	public:
		DWORD dwThreadId;
		HANDLE hThread;
		ULONG_PTR lpThreadLocalBase;
		ULONG_PTR lpStartAddress;
		RegistersInfo registers;

		/**
		\brief Default constructor.
		*/
		ThreadInfo();

		/**
		\brief Constructor.
		\param dwThreadId Identifier for the thread.
		\param lpThreadLocalBase The thread local base.
		\param lpStartAddress The start address.
		*/
		ThreadInfo(DWORD dwThreadId, HANDLE hThread, LPVOID lpThreadLocalBase, LPVOID lpStartAddress);

		/**
		\brief Read the register context from the thread. This fills the RegistersInfo member.
		\return true if it succeeds, false if it fails.
		*/
		bool RegReadContext();

		/**
		\brief Write the register context to the thread. This does nothing if the RegistersInfo member did not change.
		\return true if it succeeds, false if it fails.
		*/
		bool RegWriteContext();

	private:
		CONTEXT _oldContext;
	};
};

#endif //_DEBUGGER_THREADS_H