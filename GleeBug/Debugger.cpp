#include "Debugger.h"

namespace GleeBug
{
	Debugger::Debugger()
	{
	}

	bool Debugger::Init(const wchar_t* szFilePath,
		const wchar_t* szCommandLine,
		const wchar_t* szCurrentDirectory)
	{
		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));
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

		if (!CreateProcessW(szFileNameCreateProcess,
			szCommandLineCreateProcess,
			NULL,
			NULL,
			FALSE,
			DEBUG_PROCESS | CREATE_NEW_CONSOLE,
			NULL,
			szCurrentDirectory,
			&si,
			&pi))
		{
			return false;
		}
		_mainProcess.hProcess = pi.hProcess;
		_mainProcess.hThread = pi.hThread;
		_mainProcess.ProcessId = pi.dwProcessId;
		_mainProcess.MainThreadId = pi.dwThreadId;
		return true;
	}

	bool Debugger::Stop()
	{
		return !!TerminateProcess(_mainProcess.hProcess, 0);
	}

	bool Debugger::Detach()
	{
		return !!DebugActiveProcessStop(_mainProcess.ProcessId);
	}

	const ProcessInfo & Debugger::GetMainProcess()
	{
		return _mainProcess;
	}
};