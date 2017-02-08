#include "Debugger.h"

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
        const wchar_t* szCurrentDirectory)
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
            DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
            nullptr,
            szCurrentDirectory,
            &mMainStartupInfo,
            &mMainProcess);

        delete[] szCreateWithCmdLine;
        mAttachedToProcess = false;

        return result;
    }

    bool Debugger::Attach(DWORD processId)
    {
        //don't allow attaching when still debugging
        if(mIsDebugging)
            return false;
        if(!DebugActiveProcess(processId))
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
        mRegisters->TrapFlag = false;
        mThread->RegWriteContext();
        return !!DebugActiveProcessStop(mMainProcess.dwProcessId);
    }

    void Debugger::Detach()
    {
        mDetach = true;
        mDetachAndBreak = false;
    }

    bool Debugger::UnsafeDetachAndBreak()
    {
        if (!mProcess || !mThread || !mRegisters) //fail when there is no process or thread currently specified
            return false;

        //trigger an EXCEPTION_SINGLE_STEP in the debuggee
        mRegisters->TrapFlag = true;
        mThread->RegWriteContext();

        //detach from the process
        return UnsafeDetach();
    }

    void Debugger::DetachAndBreak()
    {
        mDetachAndBreak = true;
        mDetach = false;
    }
};