#include "TitanEngine.h"
#include <Psapi.h>
#include <TlHelp32.h>
#include <unordered_map>
#include "ntdll.h"
#include "FileMap.h"
#include <GleeBug/Static.Pe.h>
#include <GleeBug/Static.Bufferfile.h>

#pragma comment(lib, "psapi.lib")

//https://www.codeproject.com/Questions/78801/How-to-get-the-main-thread-ID-of-a-process-known-b

#ifndef MAKEULONGLONG
#define MAKEULONGLONG(ldw, hdw) ((ULONGLONG(hdw) << 32) | ((ldw) & 0xFFFFFFFF))
#endif

#ifndef MAXULONGLONG
#define MAXULONGLONG ((ULONGLONG)~((ULONGLONG)0))
#endif

using namespace GleeBug;

class Emulator
{
public:
    HINSTANCE engineHandle;

    //Debugger
    PROCESS_INFORMATION* InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
    {
        //TODO
        mCbATTACHBREAKPOINT = nullptr;
        return nullptr;
    }

    PROCESS_INFORMATION* InitDLLDebugW(const wchar_t* szFileName, bool ReserveModuleBase, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder, LPVOID EntryCallBack)
    {
        //TODO
        return nullptr;
    }

    bool StopDebug()
    {
        SetEvent(hEvent);
        return true;
    }

    static std::vector<HMODULE> enumModules(HANDLE hProcess)
    {
        std::vector<HMODULE> result;
        DWORD cbNeeded = 0;
        if(EnumProcessModules(hProcess, nullptr, 0, &cbNeeded))
        {
            result.resize(cbNeeded / sizeof(HMODULE));
            if(!EnumProcessModules(hProcess, result.data(), cbNeeded, &cbNeeded))
                result.clear();
        }
        return result;
    }

    static std::wstring getModuleName(HANDLE hProcess, HMODULE hModule)
    {
        wchar_t szFileName[MAX_PATH] = L"";
        if(!GetModuleFileNameExW(hProcess, hModule, szFileName, _countof(szFileName)))
            *szFileName = L'\0';
        return szFileName;
    }

    static MODULEINFO getModuleInfo(HANDLE hProcess, HMODULE hModule)
    {
        MODULEINFO info;
        if(!GetModuleInformation(hProcess, hModule, &info, sizeof(MODULEINFO)))
            memset(&info, 0, sizeof(info));
        return info;
    }

    void getThreadList(DWORD dwProcessId)
    {
        //https://blogs.msdn.microsoft.com/oldnewthing/20060223-14/?p=32173
        HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if(h != INVALID_HANDLE_VALUE)
        {
            THREADENTRY32 te;
            te.dwSize = sizeof(te);
            ULONGLONG ullMinCreateTime = MAXULONGLONG;
            dwMainThreadId = 0;
            if(Thread32First(h, &te))
            {
                do
                {
                    if(te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID) && te.th32OwnerProcessID == dwProcessId)
                    {
                        auto hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        mThreadList[te.th32ThreadID] = hThread;
                        FILETIME afTimes[4] = { 0 };
                        if(GetThreadTimes(hThread, &afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3]))
                        {
                            ULONGLONG ullTest = MAKEULONGLONG(afTimes[0].dwLowDateTime, afTimes[0].dwHighDateTime);
                            if(ullTest && ullTest < ullMinCreateTime)
                            {
                                ullMinCreateTime = ullTest;
                                dwMainThreadId = te.th32ThreadID;
                            }
                        }
                        else if(!dwMainThreadId)
                            dwMainThreadId = te.th32ThreadID;
                    }
                    te.dwSize = sizeof(te);
                } while(Thread32Next(h, &te));
            }
            CloseHandle(h);
        }
    }

    DWORD dwMainThreadId;
    std::unordered_map<DWORD, HANDLE> mThreadList;
    bool mIsDebugging = false;
    PVOID mEntryPoint;
    HANDLE hEvent;

    bool cleanup(bool result)
    {
        if(mProcessInfo.hProcess)
            CloseHandle(mProcessInfo.hProcess);
        if(mProcessInfo.hThread)
            CloseHandle(mProcessInfo.hThread);
        for(auto it : mThreadList)
            CloseHandle(it.second);
        mThreadList.clear();
        mIsDebugging = false;
        return result;
    }

    bool AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack)
    {
        //initialization + open process
        mCbATTACHBREAKPOINT = STEPCALLBACK(CallBack);
        mAttachProcessInfo = (PROCESS_INFORMATION*)DebugInfo;
        memset(&mProcessInfo, 0, sizeof(PROCESS_INFORMATION));
        mProcessInfo.dwProcessId = ProcessId;
        mProcessInfo.hProcess = TitanOpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
        if(!mProcessInfo.hProcess)
            return cleanup(false);

        //get threads
        getThreadList(mProcessInfo.dwProcessId);
        if(!mThreadList.count(dwMainThreadId))
            return cleanup(false);
        mProcessInfo.dwThreadId = dwMainThreadId;
        mProcessInfo.hThread = mThreadList[dwMainThreadId];
        *mAttachProcessInfo = mProcessInfo;
        
        //create process
        CREATE_PROCESS_DEBUG_INFO createProcess;
        memset(&createProcess, 0, sizeof(CREATE_PROCESS_DEBUG_INFO));
        auto mods = enumModules(mProcessInfo.hProcess);
        if(mods.empty())
            return cleanup(false);
        auto mainMod = mods[0]; //undocumented might not be always true
        auto mainName = getModuleName(mProcessInfo.hProcess, mainMod);
        auto mainInfo = getModuleInfo(mProcessInfo.hProcess, mainMod);
        if(!mainInfo.lpBaseOfDll || mainName.empty())
            return cleanup(false);
        mIsDebugging = true;
        createProcess.hFile = CreateFileW(mainName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        createProcess.hProcess = mProcessInfo.hProcess;
        createProcess.hThread = mProcessInfo.hThread;
        createProcess.lpBaseOfImage = mainMod;
        createProcess.lpStartAddress = LPTHREAD_START_ROUTINE(mEntryPoint = mainInfo.EntryPoint);
        createProcess.lpThreadLocalBase = GetTEBLocation(createProcess.hThread);
        mCbCREATEPROCESS(&createProcess);
        CloseHandle(createProcess.hFile);

        memset(&mDebugEvent, 0, sizeof(DEBUG_EVENT));
        mDebugEvent.dwProcessId = mProcessInfo.dwProcessId;
        mDebugEvent.dwThreadId = mProcessInfo.dwThreadId;

        //load modules
        for(size_t i = 1; i < mods.size(); i++)
        {
            LOAD_DLL_DEBUG_INFO loadDll;
            memset(&loadDll, 0, sizeof(LOAD_DLL_DEBUG_INFO));
            loadDll.lpBaseOfDll = mods[i];
            auto dllName = getModuleName(mProcessInfo.hProcess, mods[i]);
            if(!dllName.empty())
                loadDll.hFile = CreateFileW(dllName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            mCbLOADDLL(&loadDll);
            if(!dllName.empty())
                CloseHandle(loadDll.hFile);
        }
        
        //create threads
        for(auto it : mThreadList)
        {
            if(it.first == dwMainThreadId)
                continue;
            CREATE_THREAD_DEBUG_INFO createThread;
            memset(&createThread, 0, sizeof(CREATE_THREAD_DEBUG_INFO));
            createThread.hThread = it.second;
            ULONG len = sizeof(PVOID);
            if(NtQueryInformationThread(createThread.hThread, ThreadQuerySetWin32StartAddress, &createThread.lpStartAddress, len, &len))
                createThread.lpStartAddress = nullptr;
            createThread.lpThreadLocalBase = GetTEBLocation(createThread.hThread);
            mCbCREATETHREAD(&createThread);
        }

        //create the event that gets trigged in StopDebug
        hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        //attach breakpoint
        mCbATTACHBREAKPOINT();
        
        //system breakpoint
        mCbSYSTEMBREAKPOINT(nullptr);

        //return after stop/detach is called
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
        return cleanup(true);
    }

    bool DetachDebuggerEx(DWORD ProcessId)
    {
        //TODO
        return false;
    }

    void DebugLoop()
    {
        //TODO
    }

    void SetNextDbgContinueStatus(DWORD SetDbgCode)
    {
        //TODO
    }

    //Memory
    bool MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead) const
    {
        SIZE_T s;
        if(!lpNumberOfBytesRead)
            lpNumberOfBytesRead = &s;
        auto x = !!ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
        if (!x && nSize <= 0x1000)
        {
            NtSuspendProcess(hProcess);
            DWORD oldProtect = 0;
            if (VirtualProtectEx(hProcess, lpBaseAddress, 0x1000, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                x = !!ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
                VirtualProtectEx(hProcess, lpBaseAddress, 0x1000, oldProtect, &oldProtect);
            }
            NtResumeProcess(hProcess);
        }
        return x;
    }

    bool MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
    {
        SIZE_T s;
        if(!lpNumberOfBytesWritten)
            lpNumberOfBytesWritten = &s;
        return !!WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
    }

    bool Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte)
    {
        //TODO
        return false;
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
        if(VariableId == UE_ENGINE_SET_DEBUG_PRIVILEGE)
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

    void* GetTEBLocation(HANDLE hThread)
    {
        ULONG RequiredLen = 0;
        void* TebAddress = 0;
        THREAD_BASIC_INFORMATION myThreadBasicInformation[5] = { 0 };

        if(NtQueryInformationThread(hThread, ThreadBasicInformation, myThreadBasicInformation, sizeof(THREAD_BASIC_INFORMATION), &RequiredLen) == 0)
        {
            TebAddress = (void*)myThreadBasicInformation->TebBaseAddress;
        }
        else
        {
            if(NtQueryInformationThread(hThread, ThreadBasicInformation, myThreadBasicInformation, RequiredLen, &RequiredLen) == 0)
            {
                TebAddress = (void*)myThreadBasicInformation->TebBaseAddress;
            }
        }

        return TebAddress;
    }

    bool HideDebugger(HANDLE hProcess, DWORD PatchAPILevel)
    {
        //TODO
        return false;
    }

    HANDLE TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId)
    {
        if (mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), true);
        HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
        DWORD dwLastError = GetLastError();
        if (mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), false);
        SetLastError(dwLastError);
        return hProcess;
    }

    HANDLE TitanOpenThread(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwThreadId)
    {
        if (mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), true);
        HANDLE hThread = OpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
        DWORD dwLastError = GetLastError();
        if (mSetDebugPrivilege)
            setDebugPrivilege(GetCurrentProcess(), false);
        SetLastError(dwLastError);
        return hThread;
    }

    PROCESS_INFORMATION* TitanGetProcessInformation()
    {
        return &mProcessInfo;
    }

    //Registers
    ULONG_PTR GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister) const
    {
        switch(IndexOfRegister)
        {
        case UE_EIP:
        case UE_RIP:
        case UE_CIP:
            return ULONG_PTR(mEntryPoint);
        }
        //TODO
        return 0;
    }

    bool SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue)
    {
        //TODO
        return false;
    }

    bool GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext) const
    {
        //TODO
        titcontext->cip = ULONG_PTR(mEntryPoint);
        return true;
    }

    bool SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
    {
        //TODO
        return false;
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
        if (!file->Map(szFileName, DesiredAccess == UE_ACCESS_ALL))
            __debugbreak(); //return false;
        *FileHandle = file->hFile;
        *LoadedSize = file->size;
        *FileMap = file->hMap;
        *FileMapVA = ULONG_PTR(file->data);
        MappedPe mappedPe;
        mappedPe.file = std::move(file);
        mappedPe.buffer = new BufferFile(mappedPe.file->data, mappedPe.file->size);
        mappedPe.pe = new Pe(*mappedPe.buffer);
        if (mappedPe.pe->Parse(true) != Pe::ErrorOk)
            __debugbreak();
        mappedFiles.insert({ *FileMapVA, mappedPe });
        return true;
    }

    bool StaticFileUnloadW(const wchar_t* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA)
    {
        auto found = mappedFiles.find(FileMapVA);
        if (found == mappedFiles.end())
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
        if (found == mappedFiles.end())
            __debugbreak(); //return 0;
        if (!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        return found->second.pe->ConvertOffsetToRva(uint32(AddressToConvert));
    }

    ULONG_PTR ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType)
    {
        return ConvertVAtoFileOffsetEx(FileMapVA, 0, 0, AddressToConvert, false, ReturnType);
    }

    ULONG_PTR ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType)
    {
        auto found = mappedFiles.find(FileMapVA);
        if (found == mappedFiles.end())
            __debugbreak(); //return 0;
        if (!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        auto offset = found->second.pe->ConvertRvaToOffset(uint32(AddressToConvert));
        if (offset == INVALID_VALUE)
            return 0;
        return ReturnType ? FileMapVA + offset : offset;
    }

    template<typename T>
    ULONG_PTR GetPE32DataW_impl(const Region<T> & headers, DWORD WhichSection, DWORD WhichData, const std::vector<Section> & sections)
    {
        switch (WhichData)
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
            return (ULONG_PTR)headers->OptionalHeader.ImageBase;
        case UE_SIZEOFIMAGE:
            return headers->OptionalHeader.SizeOfImage;
        case UE_RELOCATIONTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        case UE_RELOCATIONTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        case UE_TLSTABLEADDRESS:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
        case UE_TLSTABLESIZE:
            return headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
        case UE_TIMEDATESTAMP:
            return headers->FileHeader.TimeDateStamp;
        case UE_CHECKSUM:
            return headers->OptionalHeader.CheckSum;
        case UE_SUBSYSTEM:
            return headers->OptionalHeader.Subsystem;
        case UE_NUMBEROFRVAANDSIZES:
            return headers->OptionalHeader.NumberOfRvaAndSizes;
        default:
            __debugbreak();
        }
        return 0;
    }

    ULONG_PTR GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData)
    {
        auto found = mappedFiles.find(FileMapVA);
        if (found == mappedFiles.end())
            __debugbreak(); //return 0;
        if (!found->second.pe->IsValidPe())
            __debugbreak(); //return 0;
        auto sections = found->second.pe->GetSections();
        return found->second.pe->IsPe64()
            ? GetPE32DataW_impl(found->second.pe->GetNtHeaders64(), WhichSection, WhichData, sections)
            : GetPE32DataW_impl(found->second.pe->GetNtHeaders32(), WhichSection, WhichData, sections);
    }

    ULONG_PTR GetPE32DataW(const wchar_t* szFileName, DWORD WhichSection, DWORD WhichData)
    {
        FileMap<unsigned char> file;
        if (!file.Map(szFileName))
            __debugbreak(); //return 0;
        BufferFile buf(file.data, file.size);
        Pe pe(buf);
        if (pe.Parse(true) != Pe::ErrorOk)
            __debugbreak(); //return 0;
        if (!pe.IsValidPe())
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
        //TODO
        return false;
    }

    bool DeleteBPX(ULONG_PTR bpxAddress)
    {
        //TODO
        return false;
    }

    bool IsBPXEnabled(ULONG_PTR bpxAddress)
    {
        //TODO
        return false;
    }

    void SetBPXOptions(long DefaultBreakPointType)
    {
    }

    //Memory Breakpoints
    bool SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack)
    {
        //TODO
        return false;
    }

    bool RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory)
    {
        //TODO
        return false;
    }

    //Hardware Breakpoints
    bool SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack)
    {
        //TODO
        return false;
    }

    bool DeleteHardwareBreakPoint(DWORD IndexOfRegister)
    {
        //TODO
        return false;
    }

    bool GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex)
    {
        //TODO
        return false;
    }

    //Generic Breakpoints
    bool RemoveAllBreakPoints(DWORD RemoveOption)
    {
        //TODO
        return false;
    }

    //Stepping
    void StepOver(LPVOID CallBack)
    {
        //TODO
    }

    void StepInto(LPVOID CallBack)
    {
        //TODO
    }

private: //functions
    static DWORD setDebugPrivilege(HANDLE hProcess, bool bEnablePrivilege)
    {
        DWORD dwLastError;
        HANDLE hToken = 0;
        if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            dwLastError = GetLastError();
            if (hToken)
                CloseHandle(hToken);
            return dwLastError;
        }
        TOKEN_PRIVILEGES tokenPrivileges;
        memset(&tokenPrivileges, 0, sizeof(TOKEN_PRIVILEGES));
        LUID luid;
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
        {
            dwLastError = GetLastError();
            CloseHandle(hToken);
            return dwLastError;
        }
        tokenPrivileges.PrivilegeCount = 1;
        tokenPrivileges.Privileges[0].Luid = luid;
        if (bEnablePrivilege)
            tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        else
            tokenPrivileges.Privileges[0].Attributes = 0;
        AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        dwLastError = GetLastError();
        CloseHandle(hToken);
        return dwLastError;
    }

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
    PROCESS_INFORMATION mProcessInfo;
    DEBUG_EVENT mDebugEvent;
};