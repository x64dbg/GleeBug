#include <GleeBug/Debugger.h>
#include <GleeBug/Static.Pe.h>
#include <GleeBug/Static.Bufferfile.h>
#include "TitanEngine.h"
#include "ntdll.h"
#include "FileMap.h"
#include "PEB.h"

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

    PROCESS_INFORMATION* TitanGetProcessInformation()
    {
        return &mMainProcess;
    }

    STARTUPINFOW* TitanGetStartupInformation()
    {
        return &mMainStartupInfo;
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
        memset(titcontext, 0, sizeof(TITAN_ENGINE_CONTEXT_t));
        auto context = thread->registers.GetContext();
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
        titcontext->eflags = thread->registers.Eflags();
        titcontext->gs = (unsigned short)context->SegGs;
        titcontext->fs = (unsigned short)context->SegFs;
        titcontext->es = (unsigned short)context->SegEs;
        titcontext->ds = (unsigned short)context->SegDs;
        titcontext->cs = (unsigned short)context->SegCs;
        titcontext->ss = (unsigned short)context->SegSs;
        titcontext->dr0 = thread->registers.Dr0();
        titcontext->dr1 = thread->registers.Dr1();
        titcontext->dr2 = thread->registers.Dr2();
        titcontext->dr3 = thread->registers.Dr3();
        titcontext->dr6 = thread->registers.Dr6();
        titcontext->dr7 = thread->registers.Dr7();
        return true;
    }

    bool SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        auto thread = threadFromHandle(hActiveThread);
        if (!thread || !titcontext)
            return false;
        ThreadSuspender suspender(thread, mIsRunning, true);
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
        thread->registers.Eflags = uint32(titcontext->eflags);
        thread->registers.Dr0 = titcontext->dr0;
        thread->registers.Dr1 = titcontext->dr1;
        thread->registers.Dr2 = titcontext->dr2;
        thread->registers.Dr3 = titcontext->dr3;
        thread->registers.Dr6 = titcontext->dr6;
        thread->registers.Dr7 = titcontext->dr7;
        auto context = *(thread->registers.GetContext());
        context.SegGs = titcontext->gs;
        context.SegFs = titcontext->fs;
        context.SegEs = titcontext->es;
        context.SegDs = titcontext->ds;
        context.SegCs = titcontext->cs;
        context.SegSs = titcontext->ss;
        thread->registers.SetContext(context);
        return true;
    }

    void GetMMXRegisters(uint64_t mmx[8], TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        //TODO
        memset(mmx, 0, sizeof(uint64_t) * 8);
    }

    void Getx87FPURegisters(x87FPURegister_t x87FPURegisters[8], TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        //TODO
        memset(x87FPURegisters, 0, sizeof(x87FPURegister_t) * 8);
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

    bool TLSGrabCallBackDataW(const wchar_t* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks)
    {
        //TODO
        return false;
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

    void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll, const Dll & dll) override
    {
        if (mCbLOADDLL)
            mCbLOADDLL(&loadDll);
    }

    void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll, const Dll & dll) override
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
        default:
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