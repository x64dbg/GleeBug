#include "Emulator.h"
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

Emulator emu;
static bool gSessionStarted = false;

//Debugger basics
__declspec(dllexport) void* TITCALL InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
{
    auto result = emu.InitDebugW(szFileName, szCommandLine, szCurrentFolder);
    gSessionStarted = result != nullptr;
    return result;
}

__declspec(dllexport) PROCESS_INFORMATION* TITCALL InitReplayW(const wchar_t* szArtifactPath, TitanSessionKind ExpectedKind)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return nullptr;
}

__declspec(dllexport) bool TITCALL GetSessionInfo(TITAN_SESSION_INFO* SessionInfo)
{
    if(!SessionInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    *SessionInfo = {};
    SessionInfo->structSize = sizeof(*SessionInfo);
    if(!gSessionStarted && !emu.IsFileBeingDebugged())
        return true;
    SessionInfo->kind = UE_SESSION_LIVE;
    SessionInfo->capabilities = UE_SESSION_CAP_MEMORY_READ | UE_SESSION_CAP_MEMORY_QUERY |
                                UE_SESSION_CAP_CONTEXT_READ | UE_SESSION_CAP_FORWARD_EXECUTION |
                                UE_SESSION_CAP_MEMORY_WRITE | UE_SESSION_CAP_CONTEXT_WRITE |
                                UE_SESSION_CAP_PROCESS_CONTROL | UE_SESSION_CAP_THREAD_CONTROL |
                                UE_SESSION_CAP_NATIVE_HANDLES | UE_SESSION_CAP_EXCEPTION_CONTINUE |
                                UE_SESSION_CAP_PAUSE_EXECUTION;
#ifdef _WIN64
    SessionInfo->machineType = IMAGE_FILE_MACHINE_AMD64;
#else
    SessionInfo->machineType = IMAGE_FILE_MACHINE_I386;
#endif
    return true;
}

__declspec(dllexport) bool TITCALL ReplayGetPosition(TITAN_REPLAY_POSITION* Position)
{
    if(Position)
        *Position = {};
    SetLastError(ERROR_NOT_SUPPORTED);
    return false;
}

__declspec(dllexport) bool TITCALL ReplayGetExtent(TITAN_REPLAY_POSITION* First, TITAN_REPLAY_POSITION* Last)
{
    if(First)
        *First = {};
    if(Last)
        *Last = {};
    SetLastError(ERROR_NOT_SUPPORTED);
    return false;
}

__declspec(dllexport) bool TITCALL ReplaySetPosition(const TITAN_REPLAY_POSITION* Position)
{
    SetLastError(Position ? ERROR_NOT_SUPPORTED : ERROR_INVALID_PARAMETER);
    return false;
}

__declspec(dllexport) bool TITCALL ReplayRunBack()
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return false;
}

__declspec(dllexport) bool TITCALL ReplayStepBack(TITANCBSTEP StepCallBack)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return false;
}

__declspec(dllexport) void* TITCALL InitDLLDebugW(const wchar_t* szFileName, bool ReserveModuleBase, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder, LPVOID EntryCallBack)
{
    return emu.InitDLLDebugW(szFileName, ReserveModuleBase, szCommandLine, szCurrentFolder, EntryCallBack);
}

__declspec(dllexport) bool TITCALL StopDebug()
{
    auto result = emu.StopDebug();
    gSessionStarted = false;
    return result;
}

__declspec(dllexport) bool TITCALL AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack)
{
    gSessionStarted = true;
    auto result = emu.AttachDebugger(ProcessId, KillOnExit, DebugInfo, CallBack);
    gSessionStarted = false;
    return result;
}

__declspec(dllexport) bool TITCALL DetachDebuggerEx(DWORD ProcessId)
{
    return emu.DetachDebuggerEx(ProcessId);
}

__declspec(dllexport) void TITCALL DebugLoop()
{
    emu.DebugLoop();
}

__declspec(dllexport) void TITCALL SetNextDbgContinueStatus(DWORD SetDbgCode)
{
    emu.SetNextDbgContinueStatus(SetDbgCode);
}

//Memory
__declspec(dllexport) bool TITCALL MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    return emu.MemoryReadSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

__declspec(dllexport) bool TITCALL MemoryReadUnsafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    return emu.MemoryReadUnsafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

__declspec(dllexport) bool TITCALL MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    return emu.MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

__declspec(dllexport) SIZE_T TITCALL MemoryQuerySafe(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)
{
    return VirtualQueryEx(hProcess, lpAddress, lpBuffer, dwLength);
}

__declspec(dllexport) LPVOID TITCALL MemoryAllocSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    return VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
}

__declspec(dllexport) bool TITCALL MemoryFreeSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    return !!VirtualFreeEx(hProcess, lpAddress, dwSize, dwFreeType);
}

__declspec(dllexport) bool TITCALL MemoryProtectSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
    return !!VirtualProtectEx(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
}

__declspec(dllexport) bool TITCALL Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte)
{
    return emu.Fill(MemoryStart, MemorySize, FillByte);
}

//Engine
__declspec(dllexport) bool TITCALL EngineCheckStructAlignment(DWORD StructureType, ULONG_PTR StructureSize)
{
    return emu.EngineCheckStructAlignment(StructureType, StructureSize);
}

__declspec(dllexport) bool TITCALL IsFileBeingDebugged()
{
    return emu.IsFileBeingDebugged();
}

__declspec(dllexport) void* TITCALL GetDebugData()
{
    return emu.GetDebugData();
}

__declspec(dllexport) void TITCALL SetCustomHandler(DWORD ExceptionId, LPVOID CallBack)
{
    emu.SetCustomHandler(ExceptionId, CallBack);
}

__declspec(dllexport) void TITCALL SetEngineVariable(DWORD VariableId, bool VariableSet)
{
    emu.SetEngineVariable(VariableId, VariableSet);
}

//Misc
__declspec(dllexport) void* TITCALL GetPEBLocation(HANDLE hProcess)
{
    return emu.GetPEBLocation(hProcess);
}

__declspec(dllexport) void* TITCALL GetTEBLocation(HANDLE hThread)
{
    return emu.GetTEBLocation(hThread);
}

__declspec(dllexport) bool TITCALL HideDebugger(HANDLE hProcess, DWORD PatchAPILevel)
{
    return emu.HideDebugger(hProcess, PatchAPILevel);
}

__declspec(dllexport) HANDLE TITCALL TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId)
{
    return emu.TitanOpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
}

__declspec(dllexport) HANDLE TITCALL TitanOpenThread(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwThreadId)
{
    return emu.TitanOpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
}

__declspec(dllexport) bool TITCALL TitanGetProcessImagePathW(HANDLE hProcess, LPWSTR szPath, SIZE_T cchPath)
{
    if(!hProcess || !szPath || !cchPath || cchPath > MAXDWORD)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    DWORD length = (DWORD)cchPath;
    return !!QueryFullProcessImageNameW(hProcess, 0, szPath, &length);
}

__declspec(dllexport) bool TITCALL TitanGetModulePathW(HANDLE hProcess, ULONG_PTR ModuleBase, LPWSTR szPath, SIZE_T cchPath)
{
    if(!hProcess || !ModuleBase || !szPath || !cchPath || cchPath > MAXDWORD)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    return GetModuleFileNameExW(hProcess, (HMODULE)ModuleBase, szPath, (DWORD)cchPath) != 0;
}

__declspec(dllexport) bool TITCALL TitanCloseHandle(HANDLE hEngineHandle)
{
    return !!CloseHandle(hEngineHandle);
}

__declspec(dllexport) bool TITCALL ProcessIsWow64(HANDLE hProcess, PBOOL isWow64)
{
    return !!IsWow64Process(hProcess, isWow64);
}

__declspec(dllexport) bool TITCALL TitanTerminateProcess(HANDLE hProcess, DWORD exitCode) { return !!TerminateProcess(hProcess, exitCode); }
__declspec(dllexport) bool TITCALL RequestPause(TitanPausePolicy MaximumPolicy, TITANCBPAUSE PauseCallback) { return emu.RequestPause(MaximumPolicy, PauseCallback); }
__declspec(dllexport) HANDLE TITCALL TitanCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE start, LPVOID argument, DWORD creationFlags, LPDWORD threadId) { return CreateRemoteThread(hProcess, nullptr, 0, start, argument, creationFlags, threadId); }
__declspec(dllexport) DWORD TITCALL TitanSuspendThread(HANDLE hThread) { return SuspendThread(hThread); }
__declspec(dllexport) DWORD TITCALL TitanResumeThread(HANDLE hThread) { return ResumeThread(hThread); }
__declspec(dllexport) bool TITCALL TitanTerminateThread(HANDLE hThread, DWORD exitCode) { return !!TerminateThread(hThread, exitCode); }
__declspec(dllexport) DWORD TITCALL TitanGetThreadId(HANDLE hThread) { return GetThreadId(hThread); }
__declspec(dllexport) int TITCALL TitanGetThreadPriority(HANDLE hThread) { return GetThreadPriority(hThread); }
__declspec(dllexport) bool TITCALL TitanSetThreadPriority(HANDLE hThread, int priority) { return !!SetThreadPriority(hThread, priority); }
__declspec(dllexport) bool TITCALL TitanGetThreadTimes(HANDLE hThread, LPFILETIME creation, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user) { return !!GetThreadTimes(hThread, creation, exit, kernel, user); }
__declspec(dllexport) bool TITCALL TitanQueryThreadCycleTime(HANDLE hThread, PULONG64 cycleTime) { return !!QueryThreadCycleTime(hThread, cycleTime); }

__declspec(dllexport) PROCESS_INFORMATION* TITCALL TitanGetProcessInformation()
{
    return emu.TitanGetProcessInformation();
}

static ULONG_PTR DbgValFromString(const char* expr)
{
#ifdef _WIN64
#define X64DBG_DLL L"x64dbg.dll"
#else
#define X64DBG_DLL L"x32dbg.dll"
#endif // _WIN64
    static auto hModule = GetModuleHandleW(X64DBG_DLL);
#undef X64DBG_DLL
    static auto DbgValFromString = (ULONG_PTR(*)(const char*))GetProcAddress(hModule, "DbgValFromString");
    return DbgValFromString ? DbgValFromString(expr) : 0;
}

__declspec(dllexport) ULONG_PTR TITCALL ImporterGetRemoteAPIAddressEx(const char* szDLLName, const char* szAPIName)
{
    char expr[1024] = "";
    _snprintf_s(expr, _TRUNCATE, "\"%s\":%s", szDLLName, szAPIName);
    return DbgValFromString(expr);
}

__declspec(dllexport) ULONG_PTR TITCALL GetDebuggedFileBaseAddress()
{
    return emu.GetDebuggedFileBaseAddress();
}

__declspec(dllexport) ULONG_PTR TITCALL GetDebuggedDLLBaseAddress()
{
    return DbgValFromString("mod.main()");
}

__declspec(dllexport) bool TITCALL DumpProcess(HANDLE hProcess, LPVOID ImageBase, const char* szDumpFileName, ULONG_PTR EntryPoint)
{
    // Just fails https://github.com/x64dbg/testplugin/blob/4ceae85ca8e8b63ff155495311c2c4b92febce99/test.cpp#L289, so not worth implementing
    return false;
}

//Registers
__declspec(dllexport) ULONG_PTR TITCALL GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister)
{
    return emu.GetContextDataEx(hActiveThread, IndexOfRegister);
}

__declspec(dllexport) ULONG_PTR TITCALL GetContextData(DWORD IndexOfRegister)
{
    return GetContextDataEx(TitanGetProcessInformation()->hThread, IndexOfRegister);
}

__declspec(dllexport) bool TITCALL SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue)
{
    return emu.SetContextDataEx(hActiveThread, IndexOfRegister, NewRegisterValue);
}

__declspec(dllexport) bool TITCALL GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    return emu.GetFullContextDataEx(hActiveThread, titcontext);
}

__declspec(dllexport) bool TITCALL SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    return emu.SetFullContextDataEx(hActiveThread, titcontext);
}

__declspec(dllexport) void TITCALL GetMMXRegisters(uint64_t mmx[8], TITAN_ENGINE_CONTEXT_t* titcontext)
{
    emu.GetMMXRegisters(mmx, titcontext);
}

__declspec(dllexport) void TITCALL Getx87FPURegisters(x87FPURegister_t x87FPURegisters[8], TITAN_ENGINE_CONTEXT_t* titcontext)
{
    emu.Getx87FPURegisters(x87FPURegisters, titcontext);
}

// AVX-512 constants
#ifndef XSTATE_MASK_AVX512
#define XSTATE_AVX512_KMASK                 (5)
#define XSTATE_AVX512_ZMM_H                 (6)
#define XSTATE_AVX512_ZMM                   (7)
#define XSTATE_MASK_AVX512                  ((1ui64 << (XSTATE_AVX512_KMASK)) | \
                                             (1ui64 << (XSTATE_AVX512_ZMM_H)) | \
                                             (1ui64 << (XSTATE_AVX512_ZMM)))
#endif

static bool SetAVX512ContextFallbackToAVX(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext)
{
    // Fall back to using AVX and ignore the rest
    TITAN_ENGINE_CONTEXT_t Avx;
    memset(&Avx, 0, sizeof(Avx));
    for(int i = 0; i < _countof(Avx.YmmRegisters); i++)
    {
        Avx.YmmRegisters[i] = titcontext->ZmmRegisters[i].Low;
    }
    return SetAVXContext(hActiveThread, &Avx);
}

__declspec(dllexport) bool TITCALL SetAVX512Context(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext)
{
    if(InitXState() == false)
        return false;

    DWORD64 FeatureMask = _GetEnabledXStateFeatures();
    if((FeatureMask & XSTATE_MASK_AVX512) == 0)
        return SetAVX512ContextFallbackToAVX(hActiveThread, titcontext);

    DWORD ContextSize = 0;
    BOOL Success = _InitializeContext(NULL,
                                      CONTEXT_ALL | CONTEXT_XSTATE,
                                      NULL,
                                      &ContextSize);

    if((Success == TRUE) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return false;

    std::vector<uint8_t> dataBuffer(ContextSize);

    PCONTEXT Context;
    Success = _InitializeContext(dataBuffer.data(),
                                 CONTEXT_ALL | CONTEXT_XSTATE,
                                 &Context,
                                 &ContextSize);
    if(Success == FALSE)
        return false;

    if(_SetXStateFeaturesMask(Context, XSTATE_MASK_AVX | XSTATE_MASK_AVX512) == FALSE)
        return SetAVX512ContextFallbackToAVX(hActiveThread, titcontext);

    if(GetThreadContext(hActiveThread, Context) == FALSE)
        return false;

    if(_GetXStateFeaturesMask(Context, &FeatureMask) == FALSE)
        return false;

    DWORD FeatureLengthSse;
    DWORD FeatureLengthAvx;
    DWORD FeatureLengthAvx512_KMASK;
    DWORD FeatureLengthAvx512_ZMM_H;
    DWORD FeatureLengthAvx512_ZMM;
    XmmRegister_t* Sse = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_LEGACY_SSE, &FeatureLengthSse);
    XmmRegister_t* Avx = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX, &FeatureLengthAvx);
    ULONGLONG* Avx512_KMASK = (ULONGLONG*)_LocateXStateFeature(Context, XSTATE_AVX512_KMASK, &FeatureLengthAvx512_KMASK);
    ZmmRegister_t* Avx512_ZMM = (ZmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX512_ZMM, &FeatureLengthAvx512_ZMM);
    YmmRegister_t* Avx512_ZMM_H = (YmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX512_ZMM_H, &FeatureLengthAvx512_ZMM_H);

    if(Sse != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthSse / sizeof(XmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            Sse[i] = titcontext->ZmmRegisters[i].Low.Low;
    }

    if(Avx != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx / sizeof(XmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            Avx[i] = titcontext->ZmmRegisters[i].Low.High;
    }

    if(Avx512_ZMM_H != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_ZMM_H / sizeof(YmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            Avx512_ZMM_H[i] = titcontext->ZmmRegisters[i].High;
    }

#ifdef _WIN64
    if(Avx512_ZMM != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_ZMM / sizeof(ZmmRegister_t), _countof(titcontext->ZmmRegisters) - FeatureLengthAvx / sizeof(XmmRegister_t)); i++)
            Avx512_ZMM[i] = titcontext->ZmmRegisters[i + FeatureLengthAvx / sizeof(XmmRegister_t)];
    }
#endif // _WIN64

    if(Avx512_KMASK != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_KMASK / sizeof(ULONGLONG), _countof(titcontext->Opmask)); i++)
            Avx512_KMASK[i] = titcontext->Opmask[i];
    }

    return (SetThreadContext(hActiveThread, Context) == TRUE);
}

static bool GetAVX512ContextFallbackToAVX(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext)
{
    // Fall back to using AVX and fill the rest with 0
    TITAN_ENGINE_CONTEXT_t Avx;
    memset(titcontext, 0, sizeof(*titcontext));
    if(GetAVXContext(hActiveThread, &Avx))
    {
        for(int i = 0; i < _countof(Avx.YmmRegisters); i++)
            titcontext->ZmmRegisters[i].Low = Avx.YmmRegisters[i];
        return true;
    }
    else
    {
        return false;
    }
}

__declspec(dllexport) bool TITCALL GetAVX512Context(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext)
{
    if(InitXState() == false)
        return false;

    DWORD64 FeatureMask = _GetEnabledXStateFeatures();
    if((FeatureMask & XSTATE_MASK_AVX512) == 0)  //XSTATE_MASK_AVX512
        return GetAVX512ContextFallbackToAVX(hActiveThread, titcontext);

    DWORD ContextSize = 0;
    BOOL Success = _InitializeContext(NULL,
                                      CONTEXT_ALL | CONTEXT_XSTATE,
                                      NULL,
                                      &ContextSize);

    if((Success == TRUE) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return false;

    std::vector<uint8_t> dataBuffer(ContextSize);

    PCONTEXT Context;
    Success = _InitializeContext(dataBuffer.data(),
                                 CONTEXT_ALL | CONTEXT_XSTATE,
                                 &Context,
                                 &ContextSize);
    if(Success == FALSE)
        return false;

    if(_SetXStateFeaturesMask(Context, XSTATE_MASK_AVX | XSTATE_MASK_AVX512) == FALSE)
        return GetAVX512ContextFallbackToAVX(hActiveThread, titcontext);

    if(GetThreadContext(hActiveThread, Context) == FALSE)
        return false;

    if(_GetXStateFeaturesMask(Context, &FeatureMask) == FALSE)
        return false;

    // References:
    // - https://github.com/rnpnr/raddebugger/blob/14860ad71da7d5cce7106180bd4e3feefd30e5d0/src/demon/win32/demon_core_win32.c#L826
    // - https://github.com/jdpatdiscord/ExceptionHandler/blob/f845854fcbe9ee48f141260e81f39eca37db5e26/ExceptionHandler.cpp#L382
    DWORD FeatureLengthSse;
    DWORD FeatureLengthAvx;
    DWORD FeatureLengthAvx512_KMASK;
    DWORD FeatureLengthAvx512_ZMM_H;
    DWORD FeatureLengthAvx512_ZMM;
    XmmRegister_t* Sse = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_LEGACY_SSE, &FeatureLengthSse);
    XmmRegister_t* Avx = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX, &FeatureLengthAvx);
    ULONGLONG* Avx512_KMASK = (ULONGLONG*)_LocateXStateFeature(Context, XSTATE_AVX512_KMASK, &FeatureLengthAvx512_KMASK);
    ZmmRegister_t* Avx512_ZMM = (ZmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX512_ZMM, &FeatureLengthAvx512_ZMM);
    YmmRegister_t* Avx512_ZMM_H = (YmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX512_ZMM_H, &FeatureLengthAvx512_ZMM_H);

    if(Sse != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthSse / sizeof(XmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            titcontext->ZmmRegisters[i].Low.Low = Sse[i];
    }

    if(Avx != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx / sizeof(XmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            titcontext->ZmmRegisters[i].Low.High = Avx[i];
    }

    if(Avx512_ZMM_H != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_ZMM_H / sizeof(YmmRegister_t), _countof(titcontext->ZmmRegisters)); i++)
            titcontext->ZmmRegisters[i].High = Avx512_ZMM_H[i];
    }

#ifdef _WIN64
    if(Avx512_ZMM != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_ZMM / sizeof(ZmmRegister_t), _countof(titcontext->ZmmRegisters) - FeatureLengthAvx / sizeof(XmmRegister_t)); i++)
            titcontext->ZmmRegisters[i + FeatureLengthAvx / sizeof(XmmRegister_t)] = Avx512_ZMM[i];
    }
#endif // _WIN64

    if(Avx512_KMASK != NULL)  //If the feature is unsupported by the processor it will return NULL
    {
        for(size_t i = 0; i < MIN(FeatureLengthAvx512_KMASK / sizeof(ULONGLONG), _countof(titcontext->Opmask)); i++)
            titcontext->Opmask[i] = Avx512_KMASK[i];
    }

    return true;
}

//PE
__declspec(dllexport) bool TITCALL StaticFileLoad(const char* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA)
{
    return emu.StaticFileLoad(szFileName, DesiredAccess, SimulateLoad, FileHandle, LoadedSize, FileMap, FileMapVA);
}

__declspec(dllexport) bool TITCALL StaticFileLoadW(const wchar_t* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA)
{
    return emu.StaticFileLoadW(szFileName, DesiredAccess, SimulateLoad, FileHandle, LoadedSize, FileMap, FileMapVA);
}

__declspec(dllexport) bool TITCALL StaticFileUnloadW(const wchar_t* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA)
{
    return emu.StaticFileUnloadW(szFileName, CommitChanges, FileHandle, LoadedSize, FileMap, FileMapVA);
}

__declspec(dllexport) ULONG_PTR TITCALL ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType)
{
    return emu.ConvertFileOffsetToVA(FileMapVA, AddressToConvert, ReturnType);
}

__declspec(dllexport) ULONG_PTR TITCALL ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType)
{
    return emu.ConvertVAtoFileOffset(FileMapVA, AddressToConvert, ReturnType);
}

__declspec(dllexport) ULONG_PTR TITCALL ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType)
{
    return emu.ConvertVAtoFileOffsetEx(FileMapVA, FileSize, ImageBase, AddressToConvert, AddressIsRVA, ReturnType);
}

__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData)
{
    return emu.GetPE32DataFromMappedFile(FileMapVA, WhichSection, WhichData);
}

__declspec(dllexport) ULONG_PTR TITCALL GetPE32Data(const char* szFileName, DWORD WhichSection, DWORD WhichData)
{
    return emu.GetPE32Data(szFileName, WhichSection, WhichData);
}

__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataW(const wchar_t* szFileName, DWORD WhichSection, DWORD WhichData)
{
    return emu.GetPE32DataW(szFileName, WhichSection, WhichData);
}

__declspec(dllexport) bool TITCALL IsFileDLLW(const wchar_t* szFileName, ULONG_PTR FileMapVA)
{
    return emu.IsFileDLLW(szFileName, FileMapVA);
}

//Software Breakpoints
__declspec(dllexport) bool TITCALL SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack)
{
    return emu.SetBPX(bpxAddress, bpxType, bpxCallBack);
}

__declspec(dllexport) bool TITCALL DeleteBPX(ULONG_PTR bpxAddress)
{
    return emu.DeleteBPX(bpxAddress);
}

__declspec(dllexport) bool TITCALL IsBPXEnabled(ULONG_PTR bpxAddress)
{
    return emu.IsBPXEnabled(bpxAddress);
}

__declspec(dllexport) void TITCALL SetBPXOptions(long DefaultBreakPointType)
{
    emu.SetBPXOptions(DefaultBreakPointType);
}

//Memory Breakpoints
__declspec(dllexport) bool TITCALL SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack)
{
    return emu.SetMemoryBPXEx(MemoryStart, SizeOfMemory, BreakPointType, RestoreOnHit, bpxCallBack);
}

__declspec(dllexport) bool TITCALL RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory)
{
    return emu.RemoveMemoryBPX(MemoryStart, SizeOfMemory);
}

//Hardware Breakpoints
__declspec(dllexport) bool TITCALL SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack)
{
    return emu.SetHardwareBreakPoint(bpxAddress, IndexOfRegister, bpxType, bpxSize, bpxCallBack);
}

__declspec(dllexport) bool TITCALL DeleteHardwareBreakPoint(DWORD IndexOfRegister)
{
    return emu.DeleteHardwareBreakPoint(IndexOfRegister);
}

__declspec(dllexport) bool TITCALL GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex)
{
    return emu.GetUnusedHardwareBreakPointRegister(RegisterIndex);
}

//Generic Breakpoints
__declspec(dllexport) bool TITCALL RemoveAllBreakPoints(DWORD RemoveOption)
{
    return emu.RemoveAllBreakPoints(RemoveOption);
}

//Stepping
__declspec(dllexport) void TITCALL StepOver(LPVOID traceCallBack)
{
    emu.StepOver(traceCallBack);
}

__declspec(dllexport) void TITCALL StepInto(LPVOID traceCallBack)
{
    emu.StepInto(traceCallBack);
}

BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved
)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        emu.engineHandle = hinstDLL;
    return TRUE;
}