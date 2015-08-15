#include "Debugger.Dll.h"

namespace GleeBug
{
    DllInfo::DllInfo(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint) :
        lpBaseOfDll(ptr(lpBaseOfDll)),
        sizeOfImage(sizeOfImage),
        entryPoint(ptr(entryPoint))
    {
    }
};