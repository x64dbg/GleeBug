#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"
#include "Debugger.Data.h"

namespace GleeBug
{
	/**
	\brief A debugger class.
	*/
	class Debugger
	{
	public:
		/**
		\brief Constructs the Debugger instance.
		*/
		Debugger();

		/**
		\brief Start the debuggee.
		\param szFilePath Full pathname of the file to debug.
		\param szCommandLine The command line to pass to the debuggee.
		\param szCurrentDirectory Pathname of the current directory.
		\return true if the debuggee was started correctly, false otherwise.
		*/
		bool Init(const wchar_t* szFilePath,
			const wchar_t* szCommandLine,
			const wchar_t* szCurrentDirectory);

		/**
		\brief Stops the debuggee (terminate the process)
		\return true if the debuggee was stopped correctly, false otherwise.
		*/
		bool Stop();

		/**
		\brief Detaches the debuggee.
		\return true if the debuggee was detached correctly, false otherwise.
		*/
		bool Detach();

		/**
		\brief Run the debug loop (does not return until the debuggee is detached or terminated).
		*/
		void Start();

		/**
		\brief Gets main process info.
		\return The main process info.
		*/
		const ProcessInfo & GetMainProcess();

	protected:
		/**
		\brief Process creation debug event callback. Provide an implementation to use this callback.
		\param createProcess Information about the process created.
		*/
		virtual void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO* createProcess) {};

		/**
		\brief Process termination debug event callback. Provide an implementation to use this callback.
		\param exitProcess Information about the process terminated.
		*/
		virtual void cbExitProcessEvent(const EXIT_PROCESS_DEBUG_INFO* exitProcess) {};

		/**
		\brief Thread creation debug event callback. Provide an implementation to use this callback.
		\param createThread Information about the thread created.
		*/
		virtual void cbCreateThreadEvent(const CREATE_THREAD_DEBUG_INFO* createThread) {};

		/**
		\brief Thread termination debug event callback. Provide an implementation to use this callback.
		\param exitThread Information about the thread terminated.
		*/
		virtual void cbExitThreadEvent(const EXIT_THREAD_DEBUG_INFO* exitThread) {};

		/**
		\brief DLL load debug event callback. Provide an implementation to use this callback.
		\param loadDll Information about the DLL loaded.
		*/
		virtual void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO* loadDll) {};

		/**
		\brief DLL unload debug event callback. Provide an implementation to use this callback.
		\param unloadDll Information about the DLL unloaded.
		*/
		virtual void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO* unloadDll) {};

		/**
		\brief Exception debug event callback. Provide an implementation to use this callback.
		\param exceptionInfo Information about the exception.
		*/
		virtual void cbExceptionEvent(const EXCEPTION_DEBUG_INFO* exceptionInfo) {};

		/**
		\brief Debug string debug event callback. Provide an implementation to use this callback.
		\param debugString Information about the debug string.
		*/
		virtual void cbDebugStringEvent(const OUTPUT_DEBUG_STRING_INFO* debugString) {};

		/**
		\brief RIP debug event callback. Provide an implementation to use this callback.
		\param rip Information about the RIP event.
		*/
		virtual void cbRipEvent(const RIP_INFO* rip) {};


		virtual void createProcessEvent(CREATE_PROCESS_DEBUG_INFO* createProcess);
		virtual void exitProcessEvent(EXIT_PROCESS_DEBUG_INFO* exitProcess);
		virtual void createThreadEvent(CREATE_THREAD_DEBUG_INFO* createThread);
		virtual void exitThreadEvent(EXIT_THREAD_DEBUG_INFO* exitThread);
		virtual void loadDllEvent(LOAD_DLL_DEBUG_INFO* loadDll);
		virtual void unloadDllEvent(UNLOAD_DLL_DEBUG_INFO* unloadDll);
		virtual void exceptionEvent(EXCEPTION_DEBUG_INFO* exceptionInfo);
		virtual void debugStringEvent(OUTPUT_DEBUG_STRING_INFO* debugString);
		virtual void ripEvent(RIP_INFO* rip);

		ProcessInfo _mainProcess;
		DWORD _continueStatus;
		bool _breakDebugger;
		DEBUG_EVENT _debugEvent;
	};
};

#endif //_DEBUGGER_H