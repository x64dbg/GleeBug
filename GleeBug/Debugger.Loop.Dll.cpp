#include "Debugger.h"

namespace GleeBug
{
	void Debugger::loadDllEvent(const LOAD_DLL_DEBUG_INFO & loadDll)
	{
		//DLL housekeeping
		MODULEINFO modinfo;
		memset(&modinfo, 0, sizeof(MODULEINFO));
		GetModuleInformation(_process->hProcess,
			(HMODULE)loadDll.lpBaseOfDll,
			&modinfo,
			sizeof(MODULEINFO));
		DllInfo dll(loadDll.lpBaseOfDll, modinfo.SizeOfImage, modinfo.EntryPoint);
		_process->dlls.insert({ Range(dll.lpBaseOfDll, dll.lpBaseOfDll + dll.sizeOfImage - 1), dll });

		//call the debug event callback
		cbLoadDllEvent(loadDll, dll);

		//close the file handle
		CloseHandle(loadDll.hFile);
	}

	void Debugger::unloadDllEvent(const UNLOAD_DLL_DEBUG_INFO & unloadDll)
	{
		//call the debug event callback
		ULONG_PTR lpBaseOfDll = (ULONG_PTR)unloadDll.lpBaseOfDll;
		auto dll = _process->dlls.find(Range(lpBaseOfDll, lpBaseOfDll));
		if (dll != _process->dlls.end())
			cbUnloadDllEvent(unloadDll, dll->second);
		else
			cbUnloadDllEvent(unloadDll, DllInfo(unloadDll.lpBaseOfDll, 0, 0));

		//DLL housekeeping
		if (dll != _process->dlls.end())
			_process->dlls.erase(dll);
	}
};