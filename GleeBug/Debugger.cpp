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
		_breakPoints = BreakPointManager();

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

	bool Debugger::SetBreakPointMainProcess(LPVOID address, uint32_t bp_type){
		return _breakPoints.AddBp(&_mainProcess, address, bp_type);
	}

	bool Debugger::DelBreakPointMainProcess(LPVOID address, uint32_t bp_type){
		breakpoint temp(_mainProcess.dwProcessId, address, bp_type);
		return _breakPoints.DeleteBp(&_mainProcess, temp);
	}
};