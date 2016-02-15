#ifndef DEBUGGER_DLL_H
#define DEBUGGER_DLL_H

#include "Debugger.Global.h"

namespace GleeBug
{
    /**
    \brief DLL information structure.
    */
    class Dll
    {
    public:
        ptr lpBaseOfDll;
        ptr sizeOfImage;
        ptr entryPoint;
        LOAD_DLL_DEBUG_INFO loadDllInfo;

        /**
        \brief Constructor.
        \param lpBaseOfDll The base of DLL.
        \param sizeOfImage Size of the image.
        \param entryPoint The entry point.
        \param loadDllInfo The DLL info on creation.
        */
        explicit Dll(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint, const LOAD_DLL_DEBUG_INFO & loadDllInfo);
    };
};

#endif //DEBUGGER_DLL_H