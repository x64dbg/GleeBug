#include <GleeBug/Debugger.h>
#include "TitanEngine.h"

using namespace GleeBug;

class Emulator : public Debugger
{
public:
    //Debugger
    PROCESS_INFORMATION* InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
    {
        if (!Init(szFileName, szCommandLine, szCurrentFolder))
            return nullptr;
        return &_mainProcess;
    }

    void SetNextDbgContinueStatus(DWORD SetDbgCode)
    {
        this->_continueStatus = SetDbgCode;
    }

    //Memory
    bool MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
    {
        if (!_process)
            return false;
        return _process->MemReadSafe(ptr(lpBaseAddress), lpBuffer, nSize, (ptr*)lpNumberOfBytesRead);
    }

    bool MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
    {
        if (!_process)
            return false;
        return _process->MemWriteSafe(ptr(lpBaseAddress), lpBuffer, nSize, (ptr*)lpNumberOfBytesWritten);
    }

    bool Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte)
    {
        if (!_process)
            return false;
        for (DWORD i = 0; i < MemorySize; i++)
        {
            if (!_process->MemWriteSafe(ptr(MemoryStart) + i, FillByte, 1))
                return false;
        }
        return true;
    }

    //Engine
    bool IsFileBeingDebugged() const
    {
        return _isDebugging;
    }

    DEBUG_EVENT* GetDebugData()
    {
        return &_debugEvent;
    }

    void SetCustomHandler(DWORD ExceptionId, PVOID CallBack)
    {
        switch (ExceptionId)
        {
        case UE_CH_CREATEPROCESS:
            _cbCREATEPROCESS = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_EXITPROCESS:
            _cbEXITPROCESS = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_CREATETHREAD:
            _cbCREATETHREAD = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_EXITTHREAD:
            _cbEXITTHREAD = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_SYSTEMBREAKPOINT:
            _cbSYSTEMBREAKPOINT = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_LOADDLL:
            _cbLOADDLL = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_UNLOADDLL:
            _cbUNLOADDLL = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_OUTPUTDEBUGSTRING:
            _cbOUTPUTDEBUGSTRING = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_UNHANDLEDEXCEPTION:
            _cbUNHANDLEDEXCEPTION = (CUSTOMHANDLER)CallBack;
            break;
        case UE_CH_DEBUGEVENT:
            _cbDEBUGEVENT = (CUSTOMHANDLER)CallBack;
            break;
        default:
            break;
        }
    }

    void SetEngineVariable(DWORD VariableId, bool VariableSet)
    {
        if (VariableId == UE_ENGINE_SET_DEBUG_PRIVILEGE)
            _setDebugPrivilege = VariableSet;
    }

    //Misc
    HANDLE TitanOpenProces(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId)
    {
        return OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    }

    //Stepping
    void StepOver(LPVOID CallBack) const
    {
        //TODO
        StepInto(CallBack);
    }

    void SingleStep(DWORD StepCount, LPVOID CallBack)
    {
        if (!_thread || !CallBack)
            return;
        _thread->StepInto([this, StepCount, CallBack]()
        {
            if (!StepCount)
            {
                if (CallBack)
                    ((STEPCALLBACK)CallBack)();
            }
            else
                SingleStep(StepCount - 1, CallBack);
        });
    }

    void StepInto(LPVOID CallBack) const
    {
        if (!_thread || !CallBack)
            return;
        _thread->StepInto(STEPCALLBACK(CallBack));
    }

    //Registers
    ULONG_PTR GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister) const
    {
        if (!_thread)
            return 0;
        return _thread->registers.Get(registerFromDword(IndexOfRegister));
    }

protected:
    void cbCreateProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess, const ProcessInfo & process) override
    {
        if (_cbCREATEPROCESS)
            _cbCREATEPROCESS(&createProcess);
    }

    void cbExitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess, const ProcessInfo & process) override
    {
        if (_cbEXITPROCESS)
            _cbEXITPROCESS(&exitProcess);
    }

    void cbCreateThreadEvent(const CREATE_THREAD_DEBUG_INFO & createThread, const ThreadInfo & thread) override
    {
        if (_cbCREATETHREAD)
            _cbCREATETHREAD(&createThread);
    }

    void cbExitThreadEvent(const EXIT_THREAD_DEBUG_INFO & exitThread, const ThreadInfo & thread) override
    {
        if (_cbEXITTHREAD)
            _cbEXITTHREAD(&exitThread);
    }

    void cbLoadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll, const DllInfo & dll) override
    {
        if (_cbLOADDLL)
            _cbLOADDLL(&loadDll);
    }

    void cbUnloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll, const DllInfo & dll) override
    {
        if (_cbUNLOADDLL)
            _cbUNLOADDLL(&unloadDll);
    }

    void cbUnhandledException(const EXCEPTION_RECORD & exceptionRecord, bool firstChance) override
    {
        if (_cbUNHANDLEDEXCEPTION)
            _cbUNHANDLEDEXCEPTION(&_debugEvent.u.Exception);
    }

    void cbDebugStringEvent(const OUTPUT_DEBUG_STRING_INFO & debugString) override
    {
        if (_cbOUTPUTDEBUGSTRING)
            _cbOUTPUTDEBUGSTRING(&debugString);
    }

    void cbPreDebugEvent(const DEBUG_EVENT & debugEvent) override
    {
        if (_cbDEBUGEVENT)
            _cbDEBUGEVENT(&debugEvent);
    }

    void cbSystemBreakpoint() override
    {
        if (_cbSYSTEMBREAKPOINT)
            _cbSYSTEMBREAKPOINT(&_debugEvent.u.Exception);
    }

private: //functions
    static inline Registers::R registerFromDword(DWORD IndexOfRegister)
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

private: //variables
    bool _setDebugPrivilege = false;
    typedef void(*CUSTOMHANDLER)(const void*);
    typedef void(*STEPCALLBACK)();
    CUSTOMHANDLER _cbCREATEPROCESS = nullptr;
    CUSTOMHANDLER _cbEXITPROCESS = nullptr;
    CUSTOMHANDLER _cbCREATETHREAD = nullptr;
    CUSTOMHANDLER _cbEXITTHREAD = nullptr;
    CUSTOMHANDLER _cbSYSTEMBREAKPOINT = nullptr;
    CUSTOMHANDLER _cbLOADDLL = nullptr;
    CUSTOMHANDLER _cbUNLOADDLL = nullptr;
    CUSTOMHANDLER _cbOUTPUTDEBUGSTRING = nullptr;
    CUSTOMHANDLER _cbUNHANDLEDEXCEPTION = nullptr;
    CUSTOMHANDLER _cbDEBUGEVENT = nullptr;
};