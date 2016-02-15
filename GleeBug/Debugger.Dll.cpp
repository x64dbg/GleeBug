#include "Debugger.Dll.h"

namespace GleeBug
{
    Dll::Dll(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint, const LOAD_DLL_DEBUG_INFO & loadDllInfo) :
        lpBaseOfDll(ptr(lpBaseOfDll)),
        sizeOfImage(sizeOfImage),
        entryPoint(ptr(entryPoint)),
        loadDllInfo(loadDllInfo)
    {
    }
};