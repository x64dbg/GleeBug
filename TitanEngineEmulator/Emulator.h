#include <GleeBug/Debugger.h>
#include <GleeBug/Static.Pe.h>
#include <GleeBug/Static.Bufferfile.h>
#include "TitanEngine.h"
#include "ntdll.h"
#include "FileMap.h"
#include "PEB.h"

// Related to floating x87 registers
#define GetSTInTOPStackFromStatusWord(StatusWord) ((StatusWord & 0x3800) >> 11)
#define Getx87r0PositionInRegisterArea(STInTopStack) ((8 - STInTopStack) % 8)
#define Calculatex87registerPositionInRegisterArea(x87r0_position, index) (((x87r0_position + index) % 8))
#define GetRegisterAreaOf87register(register_area, x87r0_position, index) (((char *) register_area) + 10 * Calculatex87registerPositionInRegisterArea(x87r0_position, index) )
#define GetSTValueFromIndex(x87r0_position, index) ((x87r0_position + index) % 8)

#ifdef _WIN64
//https://stackoverflow.com/a/869597/1806760
template<typename T> struct identity
{
    typedef T type;
};

template<typename Dst> Dst implicit_cast(typename identity<Dst>::type t)
{
    return t;
}

//https://github.com/electron/crashpad/blob/4054e6cba3ba023d9c00260518ec2912607ae17c/snapshot/cpu_context.cc
enum
{
    kX87TagValid = 0,
    kX87TagZero,
    kX87TagSpecial,
    kX87TagEmpty,
};

typedef uint8_t X87Register[10];

union X87OrMMXRegister
{
    struct
    {
        X87Register st;
        uint8_t st_reserved[6];
    };
    struct
    {
        uint8_t mm_value[8];
        uint8_t mm_reserved[8];
    };
};

static_assert(sizeof(X87OrMMXRegister) == sizeof(M128A), "sizeof(X87OrMMXRegister) != sizeof(M128A)");

static uint16_t FxsaveToFsaveTagWord(
    uint16_t fsw,
    uint8_t fxsave_tag,
    const X87OrMMXRegister* st_mm)
{
    // The x87 tag word (in both abridged and full form) identifies physical
    // registers, but |st_mm| is arranged in logical stack order. In order to map
    // physical tag word bits to the logical stack registers they correspond to,
    // the "stack top" value from the x87 status word is necessary.
    int stack_top = (fsw >> 11) & 0x7;

    uint16_t fsave_tag = 0;
    for(int physical_index = 0; physical_index < 8; ++physical_index)
    {
        bool fxsave_bit = (fxsave_tag & (1 << physical_index)) != 0;
        uint8_t fsave_bits;

        if(fxsave_bit)
        {
            int st_index = (physical_index + 8 - stack_top) % 8;
            const X87Register & st = st_mm[st_index].st;

            uint32_t exponent = ((st[9] & 0x7f) << 8) | st[8];
            if(exponent == 0x7fff)
            {
                // Infinity, NaN, pseudo-infinity, or pseudo-NaN. If it was important to
                // distinguish between these, the J bit and the M bit (the most
                // significant bit of |fraction|) could be consulted.
                fsave_bits = kX87TagSpecial;
            }
            else
            {
                // The integer bit the "J bit".
                bool integer_bit = (st[7] & 0x80) != 0;
                if(exponent == 0)
                {
                    uint64_t fraction = ((implicit_cast<uint64_t>(st[7]) & 0x7f) << 56) |
                        (implicit_cast<uint64_t>(st[6]) << 48) |
                        (implicit_cast<uint64_t>(st[5]) << 40) |
                        (implicit_cast<uint64_t>(st[4]) << 32) |
                        (implicit_cast<uint32_t>(st[3]) << 24) |
                        (st[2] << 16) | (st[1] << 8) | st[0];
                    if(!integer_bit && fraction == 0)
                    {
                        fsave_bits = kX87TagZero;
                    }
                    else
                    {
                        // Denormal (if the J bit is clear) or pseudo-denormal.
                        fsave_bits = kX87TagSpecial;
                    }
                }
                else if(integer_bit)
                {
                    fsave_bits = kX87TagValid;
                }
                else
                {
                    // Unnormal.
                    fsave_bits = kX87TagSpecial;
                }
            }
        }
        else
        {
            fsave_bits = kX87TagEmpty;
        }

        fsave_tag |= (fsave_bits << (physical_index * 2));
    }

    return fsave_tag;
}

static uint8_t FsaveToFxsaveTagWord(uint16_t fsave_tag)
{
    uint8_t fxsave_tag = 0;
    for(int physical_index = 0; physical_index < 8; ++physical_index)
    {
        const uint8_t fsave_bits = (fsave_tag >> (physical_index * 2)) & 0x3;
        const bool fxsave_bit = fsave_bits != kX87TagEmpty;
        fxsave_tag |= fxsave_bit << physical_index;
    }
    return fxsave_tag;
}
#endif //_WIN64

using namespace GleeBug;

class Emulator : public Debugger
{
public:
    //Debugger
    PROCESS_INFORMATION* InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
    {
        mCbATTACHBREAKPOINT = nullptr;
        if (!Init(szFileName, szCommandLine, szCurrentFolder))
            return nullptr;
        return &mMainProcess;
    }

    PROCESS_INFORMATION* InitDLLDebugW(const wchar_t* szFileName, bool ReserveModuleBase, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder, LPVOID EntryCallBack)
    {
        //TODO
        return nullptr;
    }

    bool StopDebug()
    {
        return Stop();
    }

    bool AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack)
    {
        if(!Attach(ProcessId))
            return false;
        mCbATTACHBREAKPOINT = STEPCALLBACK(CallBack);
        mAttachProcessInfo = (PROCESS_INFORMATION*)DebugInfo;
        DebugLoop();
        return true;
    }

    bool DetachDebuggerEx(DWORD ProcessId)
    {
        Detach();
        return true;
    }

    void DebugLoop()
    {
        processFromHandleCache.clear();
        threadFromHandleCache.clear();
        Start();
    }

    void SetNextDbgContinueStatus(DWORD SetDbgCode)
    {
        this->mContinueStatus = SetDbgCode;
    }

    //Memory
    bool MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
    {
        auto process = processFromHandle(hProcess);
        if (!process)
            return false;
        return process->MemReadSafe(ptr(lpBaseAddress), lpBuffer, nSize, (ptr*)lpNumberOfBytesRead);
    }

    bool MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
    {
        auto process = processFromHandle(hProcess);
        if (!process)
            return false;
        return process->MemWriteSafe(ptr(lpBaseAddress), lpBuffer, nSize, (ptr*)lpNumberOfBytesWritten);
    }

    bool Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte)
    {
        //TODO: this is fucking inefficient
        if (!mProcess)
            return false;
        for (DWORD i = 0; i < MemorySize; i++)
        {
            if (!mProcess->MemWriteSafe(ptr(MemoryStart) + i, FillByte, 1))
                return false;
        }
        return true;
    }

    //Engine
    bool EngineCheckStructAlignment(DWORD StructureType, ULONG_PTR StructureSize) const
    {
        if (StructureType == UE_STRUCT_TITAN_ENGINE_CONTEXT)
            return StructureSize == sizeof(TITAN_ENGINE_CONTEXT_t);
        return false;
    }

    bool IsFileBeingDebugged() const
    {
        return mIsDebugging;
    }

    DEBUG_EVENT* GetDebugData()
    {
        return &mDebugEvent;
    }

    void SetCustomHandler(DWORD ExceptionId, PVOID CallBack)
    {
        switch (ExceptionId)
        {
        case UE_CH_CREATEPROCESS:
            mCbCREATEPROCESS = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_EXITPROCESS:
            mCbEXITPROCESS = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_CREATETHREAD:
            mCbCREATETHREAD = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_EXITTHREAD:
            mCbEXITTHREAD = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_SYSTEMBREAKPOINT:
            mCbSYSTEMBREAKPOINT = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_LOADDLL:
            mCbLOADDLL = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_UNLOADDLL:
            mCbUNLOADDLL = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_OUTPUTDEBUGSTRING:
            mCbOUTPUTDEBUGSTRING = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_UNHANDLEDEXCEPTION:
            mCbUNHANDLEDEXCEPTION = CUSTOMHANDLER(CallBack);
            break;
        case UE_CH_DEBUGEVENT:
            mCbDEBUGEVENT = CUSTOMHANDLER(CallBack);
            break;
        default:
            break;
        }
    }

    void SetEngineVariable(DWORD VariableId, bool VariableSet)
    {
        if (VariableId == UE_ENGINE_SET_DEBUG_PRIVILEGE)
            mSetDebugPrivilege = VariableSet;
    }

    //Misc
    void* GetPEBLocation(HANDLE hProcess)
    {
        ULONG RequiredLen = 0;
        void* PebAddress = 0;
        PROCESS_BASIC_INFORMATION myProcessBasicInformation[5] = { 0 };

        if(NtQueryInformationProcess(hProcess, ProcessBasicInformation, myProcessBasicInformation, sizeof(PROCESS_BASIC_INFORMATION), &RequiredLen) == 0)
        {
            PebAddress = (void*)myProcessBasicInformation->PebBaseAddress;
        }
        else
        {
            if(NtQueryInformationProcess(hProcess, ProcessBasicInformation, myProcessBasicInformation, RequiredLen, &RequiredLen) == 0)
            {
                PebAddress = (void*)myProcessBasicInformation->PebBaseAddress;
            }
        }

        return PebAddress;
    }

    void* GetPEBLocation64(HANDLE hProcess)
    {
        void* PebAddress = 0;
#ifndef _WIN64
        if(isThisProcessWow64())
        {
            typedef NTSTATUS(WINAPI * t_NtWow64QueryInformationProcess64)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
            static auto _NtWow64QueryInformationProcess64 = (t_NtWow64QueryInformationProcess64)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtWow64QueryInformationProcess64");
            if(_NtWow64QueryInformationProcess64)
            {
                struct PROCESS_BASIC_INFORMATION64
                {
                    DWORD ExitStatus;
                    DWORD64 PebBaseAddress;
                    DWORD64 AffinityMask;
                    DWORD BasePriority;
                    DWORD64 UniqueProcessId;
                    DWORD64 InheritedFromUniqueProcessId;
                } myProcessBasicInformation[5];

                ULONG RequiredLen = 0;

                if(_NtWow64QueryInformationProcess64(hProcess, ProcessBasicInformation, myProcessBasicInformation, sizeof(PROCESS_BASIC_INFORMATION64), &RequiredLen) == 0)
                {
                    PebAddress = (void*)myProcessBasicInformation->PebBaseAddress;
                }
                else
                {
                    if(_NtWow64QueryInformationProcess64(hProcess, ProcessBasicInformation, myProcessBasicInformation, RequiredLen, &RequiredLen) == 0)
                    {
                        PebAddress = (void*)myProcessBasicInformation->PebBaseAddress;
                    }
                }
            }
        }
#endif //_WIN64
        return PebAddress;

    }

    static bool getThreadInfo(HANDLE hThread, THREAD_BASIC_INFORMATION & tbi)
    {
        ULONG RequiredLen = 0;
        THREAD_BASIC_INFORMATION myThreadBasicInformation[5] = { 0 };
        if(NtQueryInformationThread(hThread, ThreadBasicInformation, myThreadBasicInformation, sizeof(THREAD_BASIC_INFORMATION), &RequiredLen) == 0)
        {
            tbi = myThreadBasicInformation[0];
            return true;
        }
        else
        {
            if(NtQueryInformationThread(hThread, ThreadBasicInformation, myThreadBasicInformation, RequiredLen, &RequiredLen) == 0)
            {
                tbi = myThreadBasicInformation[0];
                return true;
            }
        }
        return false;
    }

    void* GetTEBLocation(HANDLE hThread)
    {
        THREAD_BASIC_INFORMATION tbi;
        return getThreadInfo(hThread, tbi) ? tbi.TebBaseAddress : nullptr;
    }

    bool HideDebugger(HANDLE hProcess, DWORD PatchAPILevel)
    {
        PEB_CURRENT myPEB = { 0 };
        SIZE_T ueNumberOfBytesRead = 0;
        void* heapFlagsAddress = 0;
        DWORD heapFlags = 0;
        void* heapForceFlagsAddress = 0;
        DWORD heapForceFlags = 0;

#ifndef _WIN64
        PEB64 myPEB64 = { 0 };
        void* AddressOfPEB64 = GetPEBLocation64(hProcess);
#endif

        void* AddressOfPEB = GetPEBLocation(hProcess);

        if(!AddressOfPEB)
            return false;

        if(ReadProcessMemory(hProcess, AddressOfPEB, (void*)&myPEB, sizeof(PEB_CURRENT), &ueNumberOfBytesRead))
        {
#ifndef _WIN64
            if(AddressOfPEB64)
            {
                ReadProcessMemory(hProcess, AddressOfPEB64, (void*)&myPEB64, sizeof(PEB64), &ueNumberOfBytesRead);
            }
#endif
            myPEB.BeingDebugged = FALSE;
            myPEB.NtGlobalFlag &= ~0x70;

#ifndef _WIN64
            myPEB64.BeingDebugged = FALSE;
            myPEB64.NtGlobalFlag &= ~0x70;
#endif

#ifdef _WIN64
            heapFlagsAddress = (void*)((LONG_PTR)myPEB.ProcessHeap + getHeapFlagsOffset(true));
            heapForceFlagsAddress = (void*)((LONG_PTR)myPEB.ProcessHeap + getHeapForceFlagsOffset(true));
#else
            heapFlagsAddress = (void*)((LONG_PTR)myPEB.ProcessHeap + getHeapFlagsOffset(false));
            heapForceFlagsAddress = (void*)((LONG_PTR)myPEB.ProcessHeap + getHeapForceFlagsOffset(false));
#endif //_WIN64
            ReadProcessMemory(hProcess, heapFlagsAddress, &heapFlags, sizeof(DWORD), 0);
            ReadProcessMemory(hProcess, heapForceFlagsAddress, &heapForceFlags, sizeof(DWORD), 0);

            heapFlags &= HEAP_GROWABLE;
            heapForceFlags = 0;

            WriteProcessMemory(hProcess, heapFlagsAddress, &heapFlags, sizeof(DWORD), 0);
            WriteProcessMemory(hProcess, heapForceFlagsAddress, &heapForceFlags, sizeof(DWORD), 0);

            if(WriteProcessMemory(hProcess, AddressOfPEB, (void*)&myPEB, sizeof(PEB_CURRENT), &ueNumberOfBytesRead))
            {
#ifndef _WIN64
                if(AddressOfPEB64)
                {
                    WriteProcessMemory(hProcess, AddressOfPEB64, (void*)&myPEB64, sizeof(PEB64), &ueNumberOfBytesRead);
                }
#endif
                return true;
            }
        }
        return false;
    }

    HANDLE TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId)
    {
        if(mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), true);
        HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
        DWORD dwLastError = GetLastError();
        if(mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), false);
        SetLastError(dwLastError);
        return hProcess;
    }

    HANDLE TitanOpenThread(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwThreadId)
    {
        if(mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), true);
        HANDLE hThread = OpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
        DWORD dwLastError = GetLastError();
        if(mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), false);
        SetLastError(dwLastError);
        return hThread;
    }

    //Stepping
    void StepOver(LPVOID CallBack)
    {
        if (!mProcess || !CallBack)
            return;
        mProcess->StepOver(STEPCALLBACK(CallBack));
    }

    void StepInto(LPVOID CallBack)
    {
        if (!mThread || !CallBack)
            return;
        mThread->StepInto(STEPCALLBACK(CallBack));
    }

    struct ThreadSuspender
    {
        ThreadSuspender(Thread* thread, bool running, bool writeRegs)
            : thread(running ? thread : nullptr), writeRegs(writeRegs)
        {
            if(this->thread)
            {
                this->thread->Suspend();
                this->thread->RegReadContext();
            }
        }

        ~ThreadSuspender()
        {
            if(this->thread)
            {
                if(this->writeRegs)
                    this->thread->RegWriteContext();
                this->thread->Resume();
            }
        }

        Thread* thread;
        bool writeRegs;
    };

    //Registers
    ULONG_PTR GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister)
    {
        if(!hActiveThread)
            return 0;
        auto thread = threadFromHandle(hActiveThread);
        if(!thread)
            return 0;
        ThreadSuspender suspender(thread, mIsRunning, false);
        return thread->registers.Get(registerFromDword(IndexOfRegister));
    }

    bool SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue)
    {
        auto thread = threadFromHandle(hActiveThread);
        if (!thread)
            return false;
        ThreadSuspender suspender(thread, mIsRunning, true);
        thread->registers.Set(registerFromDword(IndexOfRegister), NewRegisterValue);
        return true;
    }

    bool GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        if(!hActiveThread)
            return false;
        auto thread = threadFromHandle(hActiveThread);
        if (!thread || !titcontext)
            return false;
        ThreadSuspender suspender(thread, mIsRunning, false);
        auto context = thread->registers.GetContext();
        memset(titcontext, 0, sizeof(TITAN_ENGINE_CONTEXT_t));
        //General purpose registers
        titcontext->cax = thread->registers.Gax();
        titcontext->ccx = thread->registers.Gcx();
        titcontext->cdx = thread->registers.Gdx();
        titcontext->cbx = thread->registers.Gbx();
        titcontext->csp = thread->registers.Gsp();
        titcontext->cbp = thread->registers.Gbp();
        titcontext->csi = thread->registers.Gsi();
        titcontext->cdi = thread->registers.Gdi();
#ifdef _WIN64
        titcontext->r8 = thread->registers.R8();
        titcontext->r9 = thread->registers.R9();
        titcontext->r10 = thread->registers.R10();
        titcontext->r11 = thread->registers.R11();
        titcontext->r12 = thread->registers.R12();
        titcontext->r13 = thread->registers.R13();
        titcontext->r14 = thread->registers.R14();
        titcontext->r15 = thread->registers.R15();
#endif //_WIN64
        titcontext->cip = thread->registers.Gip();
        // Flags
        titcontext->eflags = thread->registers.Eflags();
        // Debug registers
        titcontext->dr0 = thread->registers.Dr0();
        titcontext->dr1 = thread->registers.Dr1();
        titcontext->dr2 = thread->registers.Dr2();
        titcontext->dr3 = thread->registers.Dr3();
        titcontext->dr6 = thread->registers.Dr6();
        titcontext->dr7 = thread->registers.Dr7();
        // Segments
        titcontext->gs = thread->registers.Gs();
        titcontext->fs = thread->registers.Fs();
        titcontext->es = thread->registers.Es();
        titcontext->ds = thread->registers.Ds();
        titcontext->cs = thread->registers.Cs();
        titcontext->ss = thread->registers.Ss();
        // x87
#ifdef _WIN64
        titcontext->x87fpu.ControlWord = context->FltSave.ControlWord;
        titcontext->x87fpu.StatusWord = context->FltSave.StatusWord;
        titcontext->x87fpu.TagWord = FxsaveToFsaveTagWord(context->FltSave.StatusWord, context->FltSave.TagWord, (const X87OrMMXRegister*)context->FltSave.FloatRegisters);
        titcontext->x87fpu.ErrorSelector = context->FltSave.ErrorSelector;
        titcontext->x87fpu.ErrorOffset = context->FltSave.ErrorOffset;
        titcontext->x87fpu.DataSelector = context->FltSave.DataSelector;
        titcontext->x87fpu.DataOffset = context->FltSave.DataOffset;
        // Skip titcontext->x87fpu.Cr0NpxState (https://github.com/x64dbg/x64dbg/issues/255)
        titcontext->MxCsr = context->MxCsr;

        for(int i = 0; i < 8; i++)
            memcpy(&titcontext->RegisterArea[i * 10], &context->FltSave.FloatRegisters[i], 10);

        for(int i = 0; i < 16; i++)
            memcpy(&titcontext->XmmRegisters[i], &context->FltSave.XmmRegisters[i], 16);
#else //x86
        titcontext->x87fpu.ControlWord = (WORD)context->FloatSave.ControlWord;
        titcontext->x87fpu.StatusWord = (WORD)context->FloatSave.StatusWord;
        titcontext->x87fpu.TagWord = (WORD)context->FloatSave.TagWord;
        titcontext->x87fpu.ErrorSelector = context->FloatSave.ErrorSelector;
        titcontext->x87fpu.ErrorOffset = context->FloatSave.ErrorOffset;
        titcontext->x87fpu.DataSelector = context->FloatSave.DataSelector;
        titcontext->x87fpu.DataOffset = context->FloatSave.DataOffset;
        titcontext->x87fpu.Cr0NpxState = context->FloatSave.Cr0NpxState;

        memcpy(titcontext->RegisterArea, context->FloatSave.RegisterArea, 80);

        // MXCSR ExtendedRegisters[24]
        memcpy(&(titcontext->MxCsr), &(context->ExtendedRegisters[24]), sizeof(titcontext->MxCsr));

        // for x86 copy the 8 Xmm Registers from ExtendedRegisters[(10+n)*16]; (n is the index of the xmm register) to the XMM register
        for(int i = 0; i < 8; i++)
            memcpy(&(titcontext->XmmRegisters[i]), &context->ExtendedRegisters[(10 + i) * 16], 16);
#endif //_WIN64

        //TODO: AVX
        return true;
    }

    bool SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        auto thread = threadFromHandle(hActiveThread);
        if (!thread || !titcontext)
            return false;
        ThreadSuspender suspender(thread, mIsRunning, true);
        auto context = thread->registers.GetContext();
        // General purpose registers
        thread->registers.Gax = titcontext->cax;
        thread->registers.Gcx = titcontext->ccx;
        thread->registers.Gdx = titcontext->cdx;
        thread->registers.Gbx = titcontext->cbx;
        thread->registers.Gsp = titcontext->csp;
        thread->registers.Gbp = titcontext->cbp;
        thread->registers.Gsi = titcontext->csi;
        thread->registers.Gdi = titcontext->cdi;
#ifdef _WIN64
        thread->registers.R8 = titcontext->r8;
        thread->registers.R9 = titcontext->r9;
        thread->registers.R10 = titcontext->r10;
        thread->registers.R11 = titcontext->r11;
        thread->registers.R12 = titcontext->r12;
        thread->registers.R13 = titcontext->r13;
        thread->registers.R14 = titcontext->r14;
        thread->registers.R15 = titcontext->r15;
#endif //_WIN64
        thread->registers.Gip = titcontext->cip;
        // Flags
        thread->registers.Eflags = uint32(titcontext->eflags);
        // Debug registers
        thread->registers.Dr0 = titcontext->dr0;
        thread->registers.Dr1 = titcontext->dr1;
        thread->registers.Dr2 = titcontext->dr2;
        thread->registers.Dr3 = titcontext->dr3;
        thread->registers.Dr6 = titcontext->dr6;
        thread->registers.Dr7 = titcontext->dr7;
        // Segments
        thread->registers.Gs = titcontext->gs;
        thread->registers.Fs = titcontext->fs;
        thread->registers.Es = titcontext->es;
        thread->registers.Ds = titcontext->ds;
        thread->registers.Cs = titcontext->cs;
        thread->registers.Ss = titcontext->ss;
        // x87
#ifdef _WIN64
        context->FltSave.ControlWord = titcontext->x87fpu.ControlWord;
        context->FltSave.StatusWord = titcontext->x87fpu.StatusWord;
        context->FltSave.TagWord = FsaveToFxsaveTagWord(titcontext->x87fpu.TagWord);
        context->FltSave.ErrorSelector = (WORD)titcontext->x87fpu.ErrorSelector;
        context->FltSave.ErrorOffset = titcontext->x87fpu.ErrorOffset;
        context->FltSave.DataSelector = (WORD)titcontext->x87fpu.DataSelector;
        context->FltSave.DataOffset = titcontext->x87fpu.DataOffset;
        // Skip titcontext->x87fpu.Cr0NpxState
        context->MxCsr = titcontext->MxCsr;

        for(int i = 0; i < 8; i++)
            memcpy(&context->FltSave.FloatRegisters[i], &(titcontext->RegisterArea[i * 10]), 10);

        for(int i = 0; i < 16; i++)
            memcpy(&(context->FltSave.XmmRegisters[i]), &(titcontext->XmmRegisters[i]), 16);
#else //x86
        context->FloatSave.ControlWord = titcontext->x87fpu.ControlWord;
        context->FloatSave.StatusWord = titcontext->x87fpu.StatusWord;
        context->FloatSave.TagWord = titcontext->x87fpu.TagWord;
        context->FloatSave.ErrorSelector = titcontext->x87fpu.ErrorSelector;
        context->FloatSave.ErrorOffset = titcontext->x87fpu.ErrorOffset;
        context->FloatSave.DataSelector = titcontext->x87fpu.DataSelector;
        context->FloatSave.DataOffset = titcontext->x87fpu.DataOffset;
        context->FloatSave.Cr0NpxState = titcontext->x87fpu.Cr0NpxState;

        memcpy(context->FloatSave.RegisterArea, titcontext->RegisterArea, 80);

        // MXCSR ExtendedRegisters[24]
        memcpy(&(context->ExtendedRegisters[24]), &titcontext->MxCsr, sizeof(titcontext->MxCsr));

        // for x86 copy the 8 Xmm Registers from ExtendedRegisters[(10+n)*16]; (n is the index of the xmm register) to the XMM register
        for(int i = 0; i < 8; i++)
            memcpy(&context->ExtendedRegisters[(10 + i) * 16], &(titcontext->XmmRegisters[i]), 16);
#endif //_WIN64
        //TODO: AVX
        return true;
    }

    void GetMMXRegisters(uint64_t mmx[8], TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        int STInTopStack = GetSTInTOPStackFromStatusWord(titcontext->x87fpu.StatusWord);
        DWORD x87r0_position = Getx87r0PositionInRegisterArea(STInTopStack);
        int i;

        for(i = 0; i < 8; i++)
            mmx[i] = *((uint64_t*)GetRegisterAreaOf87register(titcontext->RegisterArea, x87r0_position, i));
    }

    void Getx87FPURegisters(x87FPURegister_t x87FPURegisters[8], TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        /*
        GET Actual TOP register from StatusWord to order the FPUx87registers like in the FPU internal order.
        The TOP field (bits 13-11) is where the FPU keeps track of which of its 80-bit registers is at the TOP.
        The register number for the FPU's internal numbering system of the 80-bit registers would be displayed in that field.
        When the programmer specifies one of the FPU 80-bit registers ST(x) in an instruction, the FPU adds (modulo 8) the ST number
        supplied to the value in this TOP field to determine in which of its registers the required data is located.
        */

        int STInTopStack = GetSTInTOPStackFromStatusWord(titcontext->x87fpu.StatusWord);
        DWORD x87r0_position = Getx87r0PositionInRegisterArea(STInTopStack);

        for(int i = 0; i < 8; i++)
        {
            memcpy(x87FPURegisters[i].data, GetRegisterAreaOf87register(titcontext->RegisterArea, x87r0_position, i), 10);
            x87FPURegisters[i].st_value = GetSTValueFromIndex(x87r0_position, i);
            x87FPURegisters[i].tag = (int)((titcontext->x87fpu.TagWord >> (i * 2)) & 0x3);
        }
    }

    struct MappedPe
    {
        FileMap<unsigned char>* file;
        BufferFile* buffer;
        Pe* pe;
    };

    std::unordered_map<ULONG_PTR, MappedPe> mappedFiles;

    //PE
    bool StaticFileLoadW(const wchar_t* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA)
    {
        auto file = new ::FileMap<unsigned char>;
        if(!file->Map(szFileName, DesiredAccess == UE_ACCESS_ALL))
            __debugbreak(); //return false;
        *FileHandle = file->hFile;
        *LoadedSize = file->size;
        *FileMap = file->hMap;
        *FileMapVA = ULONG_PTR(file->data);
        MappedPe mappedPe;
        mappedPe.file = std::move(file);
        mappedPe.buffer = new BufferFile(mappedPe.file->data, mappedPe.file->size);
        mappedPe.pe = new Pe(*mappedPe.buffer);
        if(mappedPe.pe->Parse(true) != Pe::ErrorOk)
            __debugbreak();
        mappedFiles.insert({ *FileMapVA, mappedPe });
        return true;
    }

    bool StaticFileUnloadW(const wchar_t* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA)
    {
        auto found = mappedFiles.find(FileMapVA);
        if(found == mappedFiles.end())
            __debugbreak(); //return false;
        delete found->second.pe;
        delete found->second.buffer;
        delete found->second.file;
        mappedFiles.erase(found);
        return true;
    }

    ULONG_PTR ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType)
    {
        auto found = mappedFiles.find(FileMapVA);
        if(found == mappedFiles.end())
            __debugbreak(); //return 0;
        if(!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        return found->second.pe->ConvertOffsetToRva(uint32(AddressToConvert));
    }

    ULONG_PTR ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType)
    {
        auto found = mappedFiles.find(FileMapVA);
        if(found == mappedFiles.end())
            __debugbreak(); //return 0;
        if(!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        return found->second.pe->ConvertRvaToOffset(uint32(AddressToConvert));
    }

    template<typename T>
    ULONG_PTR GetPE32DataW_impl(const Region<T> & headers, DWORD WhichSection, DWORD WhichData, const std::vector<Section> & sections)
    {
        switch(WhichData)
        {
        case UE_PE_OFFSET:
            return headers.Offset();
        case UE_SIZEOFIMAGE:
            return headers->OptionalHeader.SizeOfImage;
        case UE_SIZEOFHEADERS:
            return headers->OptionalHeader.SizeOfHeaders;
        case UE_SECTIONALIGNMENT:
            return headers->OptionalHeader.SectionAlignment;
        case UE_IMPORTTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        case UE_IMPORTTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
        case UE_EXPORTTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        case UE_EXPORTTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        case UE_CHARACTERISTICS:
            return headers->FileHeader.Characteristics;
        case UE_DLLCHARACTERISTICS:
            return headers->OptionalHeader.DllCharacteristics;
        case UE_OEP:
            return headers->OptionalHeader.AddressOfEntryPoint;
        case UE_SECTIONNUMBER:
            return sections.size();
        case UE_SECTIONVIRTUALOFFSET: //WhichSection: IMAGE_DIRECTORY_ENTRY_EXCEPTION
            return WhichSection < sections.size() ? sections.at(WhichSection).GetHeader().VirtualAddress : 0;
        case UE_SECTIONVIRTUALSIZE: //WhichSection: IMAGE_DIRECTORY_ENTRY_EXCEPTION
            return WhichSection < sections.size() ? sections.at(WhichSection).GetHeader().Misc.VirtualSize : 0;
        case UE_SECTIONNAME:
            return WhichSection < sections.size() ? ULONG_PTR(&sections.at(WhichSection).GetHeader().Name[0]) : 0;
        case UE_IMAGEBASE:
            return headers->OptionalHeader.ImageBase;
        case UE_RELOCATIONTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        case UE_RELOCATIONTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        case UE_TLSTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
        case UE_TLSTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
        default:
            __debugbreak();
        }
        return 0;
    }

    ULONG_PTR GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData)
    {
        auto found = mappedFiles.find(FileMapVA);
        if(found == mappedFiles.end())
            __debugbreak(); //return 0;
        if(!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        auto sections = found->second.pe->GetSections();
        return found->second.pe->IsPe64()
            ? GetPE32DataW_impl(found->second.pe->GetNtHeaders64(), WhichSection, WhichData, sections)
            : GetPE32DataW_impl(found->second.pe->GetNtHeaders32(), WhichSection, WhichData, sections);
    }

    ULONG_PTR GetPE32DataW(const wchar_t* szFileName, DWORD WhichSection, DWORD WhichData)
    {
        FileMap<unsigned char> file;
        if(!file.Map(szFileName))
            __debugbreak(); //return 0;
        BufferFile buf(file.data, file.size);
        Pe pe(buf);
        if(pe.Parse(true) != Pe::ErrorOk)
            __debugbreak(); //return 0;
        if(!pe.IsValidPe())
            __debugbreak(); //return 0;
        auto sections = pe.GetSections();
        return pe.IsPe64()
            ? GetPE32DataW_impl(pe.GetNtHeaders64(), WhichSection, WhichData, sections)
            : GetPE32DataW_impl(pe.GetNtHeaders32(), WhichSection, WhichData, sections);
    }

    bool IsFileDLLW(const wchar_t* szFileName, ULONG_PTR FileMapVA)
    {
        return (GetPE32DataW(szFileName, NULL, UE_CHARACTERISTICS) & IMAGE_FILE_DLL) == IMAGE_FILE_DLL;
    }

    //Software Breakpoints
    bool SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack)
    {
        if (!mProcess)
            return false;
        return mProcess->SetBreakpoint(bpxAddress, [bpxCallBack](const BreakpointInfo &)
        {
            (BPCALLBACK(bpxCallBack))();
        }, (bpxType & UE_SINGLESHOOT) == UE_SINGLESHOOT);
    }

    bool DeleteBPX(ULONG_PTR bpxAddress)
    {
        if (!mProcess)
            return false;
        return mProcess->DeleteBreakpoint(bpxAddress);
    }

    bool IsBPXEnabled(ULONG_PTR bpxAddress)
    {
        return (mProcess->MemIsValidPtr(bpxAddress) &&
            mProcess->breakpoints.find({ BreakpointType::Software, bpxAddress }) != mProcess->breakpoints.end());
    }

    void SetBPXOptions(long DefaultBreakPointType)
    {
    }

    //Memory Breakpoints
    bool SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack)
    {
        if (!mProcess)
            return false;
        return mProcess->SetMemoryBreakpoint(ptr(MemoryStart), ptr(SizeOfMemory), [bpxCallBack](const BreakpointInfo & info)
        {
            (MEMBPCALLBACK(bpxCallBack))((const void*)info.address);
        }, memtypeFromTitan(BreakPointType), !RestoreOnHit);
    }

    bool RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory)
    {
        if (!mProcess)
            return false;
        return mProcess->DeleteMemoryBreakpoint(ptr(MemoryStart));
    }

    //Hardware Breakpoints
    bool SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack)
    {
        if (!mProcess)
            return false;
        auto running = mIsRunning;
        if(running)
        {
            for(auto & thread : mProcess->threads)
                thread.second->Suspend();
            mProcess->RegReadContext();
        }
        if(!mProcess->SetHardwareBreakpoint(bpxAddress,
            (HardwareSlot)IndexOfRegister, [bpxCallBack](const BreakpointInfo & info)
        {
            (HWBPCALLBACK(bpxCallBack))((const void*)info.address);
        }, hwtypeFromTitan(bpxType), hwsizeFromTitan(bpxSize)))
            return false;
        if(running)
        {
            mProcess->RegWriteContext();
            for(auto & thread : mProcess->threads)
                thread.second->Resume();
        }
        return true;
    }

    bool DeleteHardwareBreakPoint(DWORD IndexOfRegister)
    {
        if (!mProcess || IndexOfRegister > 3)
            return false;
        auto address = mProcess->hardwareBreakpoints[IndexOfRegister].address;
        return mProcess->DeleteHardwareBreakpoint(address);
    }

    bool GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex)
    {
        if (!mProcess || !RegisterIndex)
            return false;
        HardwareSlot slot;
        bool result = mProcess->GetFreeHardwareBreakpointSlot(slot);
        if (result)
            *RegisterIndex = (DWORD)slot;
        return result;
    }

    //Generic Breakpoints
    bool RemoveAllBreakPoints(DWORD RemoveOption)
    {
        for(auto & it : mProcesses)
        {
            auto breakpoints = it.second->breakpoints; //explicit copy
            for(const auto & jt : breakpoints)
                it.second->DeleteGenericBreakpoint(jt.second);
        }
        return false;
    }

protected:
    void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess, const Process & process) override
    {
        if (mCbCREATEPROCESS)
            mCbCREATEPROCESS(&createProcess);
    }

    void cbExitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess, const Process & process) override
    {
        if (mCbEXITPROCESS)
            mCbEXITPROCESS(&exitProcess);
    }

    void cbCreateThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread, const Thread & thread) override
    {
        if (mCbCREATETHREAD)
            mCbCREATETHREAD(&createThread);
    }

    void cbExitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread, const Thread & thread) override
    {
        if (mCbEXITTHREAD)
            mCbEXITTHREAD(&exitThread);
    }

    void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll) override
    {
        if (mCbLOADDLL)
            mCbLOADDLL(&loadDll);
    }

    void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll) override
    {
        if (mCbUNLOADDLL)
            mCbUNLOADDLL(&unloadDll);
    }

    void cbUnhandledException(const EXCEPTION_RECORD & exceptionRecord, bool firstChance) override
    {
        if (mCbUNHANDLEDEXCEPTION)
            mCbUNHANDLEDEXCEPTION(&mDebugEvent.u.Exception);
    }

    void cbDebugStringEvent(const OUTPUT_DEBUG_STRING_INFO & debugString) override
    {
        if (mCbOUTPUTDEBUGSTRING)
            mCbOUTPUTDEBUGSTRING(&debugString);
    }

    void cbPreDebugEvent(const DEBUG_EVENT & debugEvent) override
    {
        if (mCbDEBUGEVENT)
            mCbDEBUGEVENT(&debugEvent);
    }

    void cbAttachBreakpoint() override
    {
        if(mCbATTACHBREAKPOINT)
        {
            if(mAttachProcessInfo)
                *mAttachProcessInfo = mMainProcess;
            mCbATTACHBREAKPOINT();
        }
    }

    void cbSystemBreakpoint() override
    {
        if (mCbSYSTEMBREAKPOINT)
            mCbSYSTEMBREAKPOINT(&mDebugEvent.u.Exception);
    }

private: //functions
    static Registers::R registerFromDword(DWORD IndexOfRegister)
    {
        switch (IndexOfRegister)
        {
        case UE_EAX: return Registers::R::EAX;
        case UE_EBX: return Registers::R::EBX;
        case UE_ECX: return Registers::R::ECX;
        case UE_EDX: return Registers::R::EDX;
        case UE_EDI: return Registers::R::EDI;
        case UE_ESI: return Registers::R::ESI;
        case UE_EBP: return Registers::R::EBP;
        case UE_ESP: return Registers::R::ESP;
        case UE_EIP: return Registers::R::EIP;
        case UE_EFLAGS: return Registers::R::EFlags;
        case UE_DR0: return Registers::R::DR0;
        case UE_DR1: return Registers::R::DR1;
        case UE_DR2: return Registers::R::DR2;
        case UE_DR3: return Registers::R::DR3;
        case UE_DR6: return Registers::R::DR6;
        case UE_DR7: return Registers::R::DR7;
#ifdef _WIN64
        case UE_RAX: return Registers::R::RAX;
        case UE_RBX: return Registers::R::RBX;
        case UE_RCX: return Registers::R::RCX;
        case UE_RDX: return Registers::R::RDX;
        case UE_RDI: return Registers::R::RDI;
        case UE_RSI: return Registers::R::RSI;
        case UE_RBP: return Registers::R::RBP;
        case UE_RSP: return Registers::R::RSP;
        case UE_RIP: return Registers::R::RIP;
        case UE_RFLAGS: return Registers::R::EFlags;
        case UE_R8: return Registers::R::R8;
        case UE_R9: return Registers::R::R9;
        case UE_R10: return Registers::R::R10;
        case UE_R11: return Registers::R::R11;
        case UE_R12: return Registers::R::R12;
        case UE_R13: return Registers::R::R13;
        case UE_R14: return Registers::R::R14;
        case UE_R15: return Registers::R::R15;
#endif //_WIN64
        case UE_CIP: return Registers::R::GIP;
        case UE_CSP: return Registers::R::GSP;
        case UE_SEG_GS: return Registers::R::GS;
        case UE_SEG_FS: return Registers::R::FS;
        case UE_SEG_ES: return Registers::R::ES;
        case UE_SEG_DS: return Registers::R::DS;
        case UE_SEG_CS: return Registers::R::CS;
        case UE_SEG_SS: return Registers::R::SS;
        default:
            __debugbreak();
            return Registers::R::EAX;
        }
    }

    std::unordered_map<HANDLE, Thread*> threadFromHandleCache;

    Thread* threadFromHandle(HANDLE hThread)
    {
        auto found = threadFromHandleCache.find(hThread);
        if(found != threadFromHandleCache.end())
            return found->second;
        auto result = [this, hThread]() -> Thread*
        {
            THREAD_BASIC_INFORMATION tbi;
            if(!getThreadInfo(hThread, tbi))
                return nullptr;
            auto foundP = mProcesses.find(uint32(tbi.ClientId.UniqueProcess));
            if(foundP == mProcesses.end())
                return nullptr;
            auto foundT = foundP->second->threads.find(uint32(tbi.ClientId.UniqueThread));
            if(foundT == foundP->second->threads.end())
                return nullptr;
            return foundT->second.get();
        }();
        if(result)
            threadFromHandleCache[hThread] = result;
        return result;
    }

    std::unordered_map<HANDLE, Process*> processFromHandleCache;

    Process* processFromHandle(HANDLE hProcess)
    {
        auto found = processFromHandleCache.find(hProcess);
        if(found != processFromHandleCache.end())
            return found->second;
        auto result = [this, hProcess]() -> Process*
        {
            auto foundP = mProcesses.find(GetProcessId(hProcess));
            if(foundP == mProcesses.end())
                return nullptr;
            return foundP->second.get();
        }();
        if(result)
            processFromHandleCache[hProcess] = result;
        return result;
    }

    static HardwareType hwtypeFromTitan(DWORD type)
    {
        switch (type)
        {
        case UE_HARDWARE_EXECUTE:
            return HardwareType::Execute;
        case UE_HARDWARE_WRITE:
            return HardwareType::Write;
        case UE_HARDWARE_READWRITE:
            return HardwareType::Access;
        default:
            return HardwareType::Access;
        }
    }

    static HardwareSize hwsizeFromTitan(DWORD size)
    {
        switch (size)
        {
        case UE_HARDWARE_SIZE_1:
            return HardwareSize::SizeByte;
        case UE_HARDWARE_SIZE_2:
            return HardwareSize::SizeWord;
        case UE_HARDWARE_SIZE_4:
            return HardwareSize::SizeDword;
#ifdef _WIN64
        case UE_HARDWARE_SIZE_8:
            return HardwareSize::SizeQword;
#endif //_WIN64
        default:
            return HardwareSize::SizeByte;
        }
    }

    static MemoryType memtypeFromTitan(DWORD type)
    {
        switch (type)
        {
        case UE_MEMORY:
            return MemoryType::Access;
        case UE_MEMORY_READ:
            return MemoryType::Read;
        case UE_MEMORY_WRITE:
            return MemoryType::Write;
        case UE_MEMORY_EXECUTE:
            return MemoryType::Execute;
        default:
            return MemoryType::Access;
        }
    }

    static DWORD setDebugPrivilege(HANDLE hProcess, bool bEnablePrivilege)
    {
        DWORD dwLastError;
        HANDLE hToken = 0;
        if(!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            dwLastError = GetLastError();
            if(hToken)
                CloseHandle(hToken);
            return dwLastError;
        }
        TOKEN_PRIVILEGES tokenPrivileges;
        memset(&tokenPrivileges, 0, sizeof(TOKEN_PRIVILEGES));
        LUID luid;
        if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
        {
            dwLastError = GetLastError();
            CloseHandle(hToken);
            return dwLastError;
        }
        tokenPrivileges.PrivilegeCount = 1;
        tokenPrivileges.Privileges[0].Luid = luid;
        if(bEnablePrivilege)
            tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        else
            tokenPrivileges.Privileges[0].Attributes = 0;
        AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        dwLastError = GetLastError();
        CloseHandle(hToken);
        return dwLastError;
    }

    static bool isAtleastVista()
    {
        static bool isAtleastVista = false;
        static bool isSet = false;
        if(isSet)
            return isAtleastVista;
        OSVERSIONINFO versionInfo = { 0 };
        versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&versionInfo);
        isAtleastVista = versionInfo.dwMajorVersion >= 6;
        isSet = true;
        return isAtleastVista;
    }

    //Quote from The Ultimate Anti-Debugging Reference by Peter Ferrie
    //Flags field exists at offset 0x0C in the heap on the 32-bit versions of Windows NT, Windows 2000, and Windows XP; and at offset 0x40 on the 32-bit versions of Windows Vista and later.
    //Flags field exists at offset 0x14 in the heap on the 64-bit versions of Windows XP, and at offset 0x70 in the heap on the 64-bit versions of Windows Vista and later.
    //ForceFlags field exists at offset 0x10 in the heap on the 32-bit versions of Windows NT, Windows 2000, and Windows XP; and at offset 0x44 on the 32-bit versions of Windows Vista and later.
    //ForceFlags field exists at offset 0x18 in the heap on the 64-bit versions of Windows XP, and at offset 0x74 in the heap on the 64-bit versions of Windows Vista and later.
    static int getHeapFlagsOffset(bool x64)
    {
        if(x64)  //x64 offsets
        {
            if(isAtleastVista())
            {
                return 0x70;
            }
            else
            {
                return 0x14;
            }
        }
        else //x86 offsets
        {
            if(isAtleastVista())
            {
                return 0x40;
            }
            else
            {
                return 0x0C;
            }
        }
    }

    static int getHeapForceFlagsOffset(bool x64)
    {
        if(x64)  //x64 offsets
        {
            if(isAtleastVista())
            {
                return 0x74;
            }
            else
            {
                return 0x18;
            }
        }
        else //x86 offsets
        {
            if(isAtleastVista())
            {
                return 0x44;
            }
            else
            {
                return 0x10;
            }
        }
    }

#ifndef _WIN64
    static bool isThisProcessWow64()
    {
        typedef BOOL(WINAPI * tIsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
        BOOL bIsWow64 = FALSE;
        static auto fnIsWow64Process = (tIsWow64Process)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");

        if(fnIsWow64Process)
        {
            fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
        }

        return (bIsWow64 != FALSE);
    }
#endif

private: //variables
    bool mSetDebugPrivilege = false;
    typedef void(*CUSTOMHANDLER)(const void*);
    typedef void(*STEPCALLBACK)();
    typedef STEPCALLBACK BPCALLBACK;
    typedef CUSTOMHANDLER HWBPCALLBACK;
    typedef CUSTOMHANDLER MEMBPCALLBACK;
    CUSTOMHANDLER mCbCREATEPROCESS = nullptr;
    CUSTOMHANDLER mCbEXITPROCESS = nullptr;
    CUSTOMHANDLER mCbCREATETHREAD = nullptr;
    CUSTOMHANDLER mCbEXITTHREAD = nullptr;
    CUSTOMHANDLER mCbSYSTEMBREAKPOINT = nullptr;
    CUSTOMHANDLER mCbLOADDLL = nullptr;
    CUSTOMHANDLER mCbUNLOADDLL = nullptr;
    CUSTOMHANDLER mCbOUTPUTDEBUGSTRING = nullptr;
    CUSTOMHANDLER mCbUNHANDLEDEXCEPTION = nullptr;
    CUSTOMHANDLER mCbDEBUGEVENT = nullptr;
    STEPCALLBACK mCbATTACHBREAKPOINT = nullptr;
    PROCESS_INFORMATION* mAttachProcessInfo = nullptr;
};