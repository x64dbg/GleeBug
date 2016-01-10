#include "Debugger.Dll.h"

namespace GleeBug
{
    Dll::Dll(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint) :
        lpBaseOfDll(ptr(lpBaseOfDll)),
        sizeOfImage(sizeOfImage),
        entryPoint(ptr(entryPoint))
    {
    }
};