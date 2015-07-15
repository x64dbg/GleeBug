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
        \brief Default constructor.
        */
        DllInfo();

        /**
        \brief Constructor.
        \param lpBaseOfDll The base of DLL.
        \param sizeOfImage Size of the image.
        \param entryPoint The entry point.
        */
        DllInfo(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint);
    };
};

#endif //_DEBUGGER_DLL_H