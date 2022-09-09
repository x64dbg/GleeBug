#include "Debugger.NativeAttach.h"
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
        if(szCommandLine == nullptr || !wcslen(szCommandLine))
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

        auto creationFlags = DEBUG_PROCESS;
        creationFlags |= DEBUG_ONLY_THIS_PROCESS; // TODO: support child process debugging
        if (newConsole)
            creationFlags |= CREATE_NEW_CONSOLE;
        if (startSuspended)
            creationFlags |= CREATE_SUSPENDED;

        if (mDisableAslr)
        {
            creationFlags |= CREATE_SUSPENDED;
            // We will attach manually later
            creationFlags &= ~(DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS);
        }

        bool result = !!CreateProcessW(szFileNameCreateProcess,
                                       szCommandLineCreateProcess,
                                       nullptr,
                                       nullptr,
                                       FALSE,
                                       creationFlags,
                                       nullptr,
                                       szCurrentDirectory,
                                       &mMainStartupInfo,
                                       &mMainProcess);

        if (result && mDisableAslr)
        {
            HollowProcessWithoutASLR(szFileNameCreateProcess, mMainProcess);
            DebugActiveProcess_(mMainProcess.dwProcessId);
            DebugSetProcessKillOnExit(TRUE);
            if (!startSuspended)
                ResumeThread(mMainProcess.hThread);
        }

        delete[] szCreateWithCmdLine;
        mAttachedToProcess = false;

        return result;
    }

    bool Debugger::Attach(DWORD processId)
    {
        //don't allow attaching when still debugging
        if(mIsDebugging)
            return false;
        if(!DebugActiveProcess_(processId))
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
        // TODO: remove from all threads?
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
        if(!mProcess || !mThread)  //fail when there is no process or thread currently specified
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

    static bool GetPeData(HANDLE hFile, ULONG_PTR& imageBase, ULONG_PTR& entryPoint)
    {
        IMAGE_DOS_HEADER idh;
        DWORD read = 0;
        if (!ReadFile(hFile, &idh, sizeof(idh), &read, nullptr))
            return false;
        if (idh.e_magic != IMAGE_DOS_SIGNATURE)
            return false;
        if (!SetFilePointer(hFile, idh.e_lfanew, nullptr, FILE_BEGIN))
            return false;
        IMAGE_NT_HEADERS64 inth;
        if (!ReadFile(hFile, &inth, sizeof(inth), &read, nullptr))
            return false;
        if (inth.Signature != IMAGE_NT_SIGNATURE)
            return false;
        switch (inth.FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_AMD64:
        {
            imageBase = inth.OptionalHeader.ImageBase;
            entryPoint = inth.OptionalHeader.AddressOfEntryPoint;
        }
        break;

        case IMAGE_FILE_MACHINE_I386:
        {
            auto nth32 = (IMAGE_NT_HEADERS32*)&inth;
            imageBase = nth32->OptionalHeader.ImageBase;
            entryPoint = nth32->OptionalHeader.AddressOfEntryPoint;
        }
        break;

        default:
            return false;
        }
        return true;
    }

    bool Debugger::HollowProcessWithoutASLR(const wchar_t* szFileName, PROCESS_INFORMATION& pi)
    {
        bool success = false;
        auto hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Retrieve image base and entry point
            ULONG_PTR debugModuleEntryPoint = 0;
            if (GetPeData(hFile, mDebugModuleImageBase, debugModuleEntryPoint))
            {
                SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

                auto hMapping = CreateFileMappingW(hFile, nullptr, SEC_IMAGE | PAGE_READONLY, 0, 0, nullptr);
                if (hMapping)
                {
                    CONTEXT ctx;
                    ctx.ContextFlags = CONTEXT_ALL;
                    if (GetThreadContext(pi.hThread, &ctx))
                    {
                        PVOID imageBase;
                        // TODO: support wow64 processes
#ifdef _WIN64
                        auto& pebRegister = ctx.Rdx;
                        auto& entryPointRegister = ctx.Rcx;
#else
                        auto& pebRegister = ctx.Ebx;
                        auto& entryPointRegister = ctx.Eax;
#endif // _WIN64
                        if (ReadProcessMemory(pi.hProcess, (char*)pebRegister + offsetof(PEB, ImageBaseAddress), &imageBase, sizeof(PVOID), nullptr))
                        {
                            auto status = NtUnmapViewOfSection(pi.hProcess, imageBase);
                            if (status == STATUS_SUCCESS)
                            {
                                SIZE_T viewSize = 0;
                                imageBase = PVOID(mDebugModuleImageBase);
                                status = NtMapViewOfSection(hMapping, pi.hProcess, &imageBase, 0, 0, nullptr, &viewSize, ViewUnmap, 0, PAGE_READONLY);
                                if (status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE)
                                {
                                    if (WriteProcessMemory(pi.hProcess, (char*)pebRegister + offsetof(PEB, ImageBaseAddress), &imageBase, sizeof(PVOID), nullptr))
                                    {
                                        entryPointRegister = mDebugModuleImageBase + debugModuleEntryPoint;
                                        if (SetThreadContext(pi.hThread, &ctx))
                                        {
                                            success = true;
#ifndef _WIN64
                                            // For Wow64 processes, also adjust the 64-bit PEB
                                            if (IsThisProcessWow64() && !WriteProcessMemory(pi.hProcess, (char*)pebRegister - 0x1000 + 0x10, &imageBase, sizeof(PVOID), nullptr))
                                                success = false;
#endif // _WIN64
                                        }
                                    }
                                }
                            }
                        }
                    }

                    CloseHandle(hMapping);
                }
            }

            CloseHandle(hFile);
        }

        if (!success)
        {
            mDebugModuleImageBase = 0;
        }

        return success;
    }
};