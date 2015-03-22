#include "Debugger.Core.h"
#include <windows.h>

namespace Debugger
{
	static DebugState state;

	bool Init(const wchar_t* szFilePath,
		const wchar_t* szCommandLine,
		const wchar_t* szCurrentDirectory,
		ProcessInfo* process)
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
		state.Process.hProcess = pi.hProcess;
		state.Process.hThread = pi.hThread;
		state.Process.ProcessId = pi.dwProcessId;
		state.Process.MainThreadId = pi.dwThreadId;
		if (process)
			*process = state.Process;
		return true;
	}

	bool Stop()
	{
		return !!TerminateProcess(state.Process.hProcess, 0);
	}

	bool Detach()
	{
		return !!DebugActiveProcessStop(state.Process.ProcessId);
	}

	DebugState* State()
	{
		return &state;
	}
};