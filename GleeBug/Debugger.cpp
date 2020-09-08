#include "Debugger.h"
#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
    Debugger::Debugger()
    {
        mProcesses.clear();
    }

    Debugger::~Debugger()
    {
    }

    bool Debugger::Init(const wchar_t* szFilePath,
        const wchar_t* szCommandLine,
        const wchar_t* szCurrentDirectory,
        bool newConsole,
        bool startSuspended)
    {
        memset(&mMainStartupInfo, 0, sizeof(mMainStartupInfo));
        memset(&mMainProcess, 0, sizeof(mMainProcess));
        const wchar_t* szFileNameCreateProcess;
        wchar_t* szCommandLineCreateProcess;
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
            DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS | (newConsole ? CREATE_NEW_CONSOLE : 0) | (startSuspended ? CREATE_SUSPENDED : 0),
            nullptr,
            szCurrentDirectory,
            &mMainStartupInfo,
            &mMainProcess);

        delete[] szCreateWithCmdLine;
        mAttachedToProcess = false;

        return result;
    }

    bool Debugger::Attach(DWORD processId, decltype(&DebugActiveProcess) debugActiveProcess)
    {
        //don't allow attaching when still debugging
        if(mIsDebugging)
            return false;
        if(!debugActiveProcess(processId))
            return false;
        mAttachedToProcess = true;
        memset(&mMainStartupInfo, 0, sizeof(mMainStartupInfo));
        memset(&mMainProcess, 0, sizeof(mMainProcess));
        return true;
    }

    bool Debugger::Stop() const
    {
        return !!TerminateProcess(mMainProcess.hProcess, 0);
    }

    bool Debugger::UnsafeDetach()
    {
        Registers(mThread->hThread, CONTEXT_CONTROL).TrapFlag = false;
        return !!DebugActiveProcessStop(mMainProcess.dwProcessId);
    }

    void Debugger::Detach()
    {
        mDetach = true;
        mDetachAndBreak = false;
    }

    bool Debugger::UnsafeDetachAndBreak()
    {
        if (!mProcess || !mThread) //fail when there is no process or thread currently specified
            return false;

        //trigger an EXCEPTION_SINGLE_STEP in the debuggee
        Registers(mThread->hThread, CONTEXT_CONTROL).TrapFlag = true;

        //detach from the process
        return UnsafeDetach();
    }

    void Debugger::DetachAndBreak()
    {
        mDetachAndBreak = true;
        mDetach = false;
    }
};