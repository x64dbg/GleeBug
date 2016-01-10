#include "Debugger.h"

namespace GleeBug
{
    void Debugger::loadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll)
    {
        //DLL housekeeping
        MODULEINFO modinfo;
        memset(&modinfo, 0, sizeof(MODULEINFO));
        GetModuleInformation(mProcess->hProcess,
            HMODULE(loadDll.lpBaseOfDll),
            &modinfo,
            sizeof(MODULEINFO));
        Dll dll(loadDll.lpBaseOfDll, modinfo.SizeOfImage, modinfo.EntryPoint);
        mProcess->dlls.insert({ Range(dll.lpBaseOfDll, dll.lpBaseOfDll + dll.sizeOfImage - 1), dll });

        //call the debug event callback
        cbLoadDllEvent(loadDll, dll);

        //close the file handle
        CloseHandle(loadDll.hFile);
    }

    void Debugger::unloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll)
    {
        //call the debug event callback
        ptr lpBaseOfDll = ptr(unloadDll.lpBaseOfDll);
        auto dll = mProcess->dlls.find(Range(lpBaseOfDll, lpBaseOfDll));
        if (dll != mProcess->dlls.end())
            cbUnloadDllEvent(unloadDll, dll->second);
        else
            cbUnloadDllEvent(unloadDll, Dll(unloadDll.lpBaseOfDll, 0, nullptr));

        //DLL housekeeping
        if (dll != mProcess->dlls.end())
            mProcess->dlls.erase(dll);
    }
};