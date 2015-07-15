#include "Debugger.h"

namespace GleeBug
{
    Debugger::Debugger()
    {
        _processes.clear();
    }

    Debugger::~Debugger()
    {
    }

    bool Debugger::Init(const wchar_t* szFilePath,
        const wchar_t* szCommandLine,
        const wchar_t* szCurrentDirectory)
    {
        STARTUPINFOW si;
        memset(&si, 0, sizeof(si));
        const wchar_t* szFileNameCreateProcess;
        wchar_t* szCommandLineCreateProcess = nullptr;
        wchar_t* szCreateWithCmdLine = nullptr;
        if (szCommandLine == nullptr || !wcslen(szCommandLine))
        {
            szCommandLineCreateProcess = nullptr;
            szFileNameCreateProcess = szFilePath;
        }
        else
        {
            auto size = 1 + wcslen(szFilePath) + 2 + wcslen(szCommandLine) + 1;
            szCreateWithCmdLine = new wchar_t[size];
            swprintf_s(szCreateWithCmdLine, size, L"\"%s\" %s", szFilePath, szCommandLine);
            szCommandLineCreateProcess = szCreateWithCmdLine;
            szFileNameCreateProcess = nullptr;
        }

        bool result = !!CreateProcessW(szFileNameCreateProcess,
            szCommandLineCreateProcess,
            nullptr,
            nullptr,
            FALSE,
            DEBUG_PROCESS | CREATE_NEW_CONSOLE,
            nullptr,
            szCurrentDirectory,
            &si,
            &_mainProcess);

        if (szCreateWithCmdLine)
            delete[] szCreateWithCmdLine;

        return result;
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