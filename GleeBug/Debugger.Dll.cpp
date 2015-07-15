#include "Debugger.Dll.h"

namespace GleeBug
{
    DllInfo::DllInfo()
    {
    }

    DllInfo::DllInfo(LPVOID lpBaseOfDll, ULONG_PTR sizeOfImage, LPVOID entryPoint)
    {
        this->lpBaseOfDll = reinterpret_cast<ULONG_PTR>(lpBaseOfDll);
        this->sizeOfImage = sizeOfImage;
        this->entryPoint = reinterpret_cast<ULONG_PTR>(entryPoint);
    }
};