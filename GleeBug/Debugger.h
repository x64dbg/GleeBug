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
		void createProcessEvent(CREATE_PROCESS_DEBUG_INFO* CreateProcess);
		void exitProcessEvent(EXIT_PROCESS_DEBUG_INFO* ExitProcess);
		void createThreadEvent(CREATE_THREAD_DEBUG_INFO* CreateThread);
		void exitThreadEvent(EXIT_THREAD_DEBUG_INFO* ExitThread);
		void loadDllEvent(LOAD_DLL_DEBUG_INFO* LoadDll);
		void unloadDllEvent(UNLOAD_DLL_DEBUG_INFO* UnloadDll);
		void exceptionEvent(EXCEPTION_DEBUG_INFO* Exception);
		void debugStringEvent(OUTPUT_DEBUG_STRING_INFO* DebugString);
		void ripEvent(RIP_INFO* Rip);
		void log(std::string msg);

	private:
		ProcessInfo _mainProcess;
		DWORD _continueStatus;
		bool _breakDebugger;
		DEBUG_EVENT _debugEvent;
	};
};

#endif //_DEBUGGER_H