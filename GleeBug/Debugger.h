#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "_global.h"

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

	private:
		//state variables
	};
};

#endif //_DEBUGGER_H