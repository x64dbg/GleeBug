#include "Debugger.Dll.h"

namespace GleeBug
{
	DllInfo::DllInfo()
	{
	}

	DllInfo::DllInfo(LPVOID lpBaseOfDll, DWORD sizeOfImage, LPVOID entryPoint)
	{
		this->lpBaseOfDll = (ULONG_PTR)lpBaseOfDll;
		this->sizeOfImage = sizeOfImage;
		this->entryPoint = (ULONG_PTR)entryPoint;
	}
};