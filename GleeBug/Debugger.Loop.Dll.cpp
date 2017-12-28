#include "Debugger.h"

namespace GleeBug
{
    void Debugger::loadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll)
    {
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