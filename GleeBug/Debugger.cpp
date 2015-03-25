#include "Debugger.h"

namespace GleeBug
{
	Debugger::Debugger()
	{
		_processes.clear();
	}

	bool Debugger::Init(const wchar_t* szFilePath,
		const wchar_t* szCommandLine,
		const wchar_t* szCurrentDirectory)
	{
		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		const wchar_t* szFileNameCreateProcess;
		wchar_t* szCommandLineCreateProcess;
		if (szCommandLine == NULL || !wcslen(szCommandLine))
		{
			szCommandLineCreateProcess = 0;
			szFileNameCreateProcess = szFilePath;
		}
		else
		{
			wchar_t szCreateWithCmdLine[1024];
			swprintf_s(szCreateWithCmdLine, L"\"%s\" %s", szFilePath, szCommandLine);
			szCommandLineCreateProcess = szCreateWithCmdLine;
			szFileNameCreateProcess = 0;
		}

		return !!CreateProcessW(szFileNameCreateProcess,
			szCommandLineCreateProcess,
			NULL,
			NULL,
			FALSE,
			DEBUG_PROCESS | CREATE_NEW_CONSOLE,
			NULL,
			szCurrentDirectory,
			&si,
			&_mainProcess);
	}

	bool Debugger::Stop()
	{
		return !!TerminateProcess(_mainProcess.hProcess, 0);
	}

	bool Debugger::Detach()
	{
		return !!DebugActiveProcessStop(_mainProcess.dwProcessId);
	}
};