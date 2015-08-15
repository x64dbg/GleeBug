#ifndef _DEBUGGER_DLL_H
#define _DEBUGGER_DLL_H

#include "Debugger.Global.h"

namespace GleeBug
{
    /**
    \brief DLL information structure.
    */
    class DllInfo
    {
    public:
        ptr lpBaseOfDll;
        ptr sizeOfImage;
        ptr entryPoint;

        /**
        \brief Constructor.
        \param lpBaseOfDll The base of DLL.
        \param sizeOfImage Size of the image.
        \param entryPoint The entry point.
        */
        explicit DllInfo(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint);
    };
};

#endif //_DEBUGGER_DLL_H