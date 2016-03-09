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
        STARTUPINFOW si;
        memset(&si, 0, sizeof(si));
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
            DEBUG_PROCESS | CREATE_NEW_CONSOLE,
            nullptr,
            szCurrentDirectory,
            &si,
            &mMainProcess);

        delete[] szCreateWithCmdLine;

        return result;
    }

    bool Debugger::Stop() const
    {
        return !!TerminateProcess(mMainProcess.hProcess, 0);
    }

    bool Debugger::UnsafeDetach()
    {
        return !!DebugActiveProcessStop(mMainProcess.dwProcessId);
    }

    void Debugger::Detach()
    {
        mDetach = true;
        mDetachAndBreak = false;
    }

    bool Debugger::UnsafeDetachAndBreak() //TODO check with child processes
    {
        if (!mProcess || !mThread || !mRegisters) //fail when there is no process or thread currently specified
            return false;
        
        //set the trap flag to trigger an exception
        auto gip = mRegisters->Gip();
        auto codePtr = ptr(VirtualAllocEx(mProcess->hProcess, nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        unsigned char code[2] = { 0xCC, 0xC3 };
        mProcess->MemWriteUnsafe(codePtr, code, sizeof(code));

        mRegisters->Gsp -= sizeof(ptr);
        mProcess->MemWriteUnsafe(mRegisters->Gsp(), &gip, sizeof(gip));
        mRegisters->Gip = codePtr;
        mThread->RegWriteContext();
        
        //detach from the process
        return UnsafeDetach();
    }

    void Debugger::DetachAndBreak()
    {
        mDetachAndBreak = true;
        mDetach = false;

        //unset the trap flag when set by GleeBug
        if (mThread->isInternalStepping || mThread->isSingleStepping)
            mRegisters->TrapFlag = false;
    }
};