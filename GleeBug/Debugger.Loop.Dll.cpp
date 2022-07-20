#include "Debugger.h"

namespace GleeBug
{
    void Debugger::loadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll)
    {
        //get process DEP policy (right opportunity)
        /*
        PspUserThreadStartup->
        DbgkCreateThread->PS_PROCESS_FLAGS_CREATE_REPORTED->DbgkpSendApiMessage->DbgkpQueueMessage
        PspInitializeThunkContext->PspSetContextThreadInternal->PspGetSetContextSpecialApc->KeContextToKframes

        DbgkpQueueMessage->
            ntdll.WaitForDebugEvent->NtWaitForDebugEvent->DbgUiConvertStateChangeStructure->CREATE_PROCESS_DEBUG_EVENT

        KeContextToKframes->
            ntdll.LdrInitializeThunk->
                ntdll.LdrpInitialize->
                    ntdll.LdrpInitializeProcess->
                        ntdll.RtlQueryImageFileKeyOption->
                            ntdll.ZwSetInformationProcess(0x22) dep flags
        */
#ifndef _WIN64
        typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
            _In_  HANDLE  /*hProcess*/,
            _Out_ LPDWORD /*lpFlags*/,
            _Out_ PBOOL   /*lpPermanent*/
        );
        static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));
        if(GPDP)
        {
            //If you use mProcess->hProcess GetProcessDEPPolicy will put garbage in bPermanent.
            auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mProcess->dwProcessId);
            DWORD lpFlags;
            BOOL bPermanent;
            if(GPDP(hProcess, &lpFlags, &bPermanent))
                mProcess->permanentDep = lpFlags != 0 && bPermanent;
            CloseHandle(hProcess);
        }
#else
        mProcess->permanentDep = true;
#endif //_WIN64

        //call the debug event callback
        cbLoadDllEvent(loadDll);

        //close the file handle
        if(loadDll.hFile)
            CloseHandle(loadDll.hFile);
    }

    void Debugger::unloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll)
    {
        //call the debug event callback
        cbUnloadDllEvent(unloadDll);
    }
};