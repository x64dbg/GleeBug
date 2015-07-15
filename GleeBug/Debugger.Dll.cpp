#include "Debugger.Dll.h"

namespace GleeBug
{
    DllInfo::DllInfo(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint)
    {
        this->lpBaseOfDll = ptr(lpBaseOfDll);
        this->sizeOfImage = sizeOfImage;
        this->entryPoint = ptr(entryPoint);
    }
};