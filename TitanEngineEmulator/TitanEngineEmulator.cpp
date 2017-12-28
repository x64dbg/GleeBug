#include <windows.h>
#include "Emulator.h"

Emulator emu;

//Debugger basics
__declspec(dllexport) void* TITCALL InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
{
    return emu.InitDebugW(szFileName, szCommandLine, szCurrentFolder);
}

__declspec(dllexport) void* TITCALL InitDLLDebugW(const wchar_t* szFileName, bool ReserveModuleBase, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder, LPVOID EntryCallBack)
{
    return emu.InitDLLDebugW(szFileName, ReserveModuleBase, szCommandLine, szCurrentFolder, EntryCallBack);
}

__declspec(dllexport) bool TITCALL StopDebug()
{
    return emu.StopDebug();
}

__declspec(dllexport) bool TITCALL AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack)
{
    return emu.AttachDebugger(ProcessId, KillOnExit, DebugInfo, CallBack);
}

__declspec(dllexport) bool TITCALL DetachDebuggerEx(DWORD ProcessId)
{
    return emu.DetachDebuggerEx(ProcessId);
}

__declspec(dllexport) void TITCALL DebugLoop()
{
    emu.DebugLoop();
}

__declspec(dllexport) void TITCALL SetNextDbgContinueStatus(DWORD SetDbgCode)
{
    emu.SetNextDbgContinueStatus(SetDbgCode);
}

//Memory
__declspec(dllexport) bool TITCALL MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    return emu.MemoryReadSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

__declspec(dllexport) bool TITCALL MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    return emu.MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

__declspec(dllexport) bool TITCALL Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte)
{
    return emu.Fill(MemoryStart, MemorySize, FillByte);
}

//Engine
__declspec(dllexport) bool TITCALL EngineCheckStructAlignment(DWORD StructureType, ULONG_PTR StructureSize)
{
    return emu.EngineCheckStructAlignment(StructureType, StructureSize);
}

__declspec(dllexport) bool TITCALL IsFileBeingDebugged()
{
    return emu.IsFileBeingDebugged();
}

__declspec(dllexport) void* TITCALL GetDebugData()
{
    return emu.GetDebugData();
}

__declspec(dllexport) void TITCALL SetCustomHandler(DWORD ExceptionId, LPVOID CallBack)
{
    emu.SetCustomHandler(ExceptionId, CallBack);
}

__declspec(dllexport) void TITCALL SetEngineVariable(DWORD VariableId, bool VariableSet)
{
    emu.SetEngineVariable(VariableId, VariableSet);
}

//Misc
__declspec(dllexport) void* TITCALL GetPEBLocation(HANDLE hProcess)
{
    return emu.GetPEBLocation(hProcess);
}

__declspec(dllexport) void* TITCALL GetTEBLocation(HANDLE hThread)
{
    return emu.GetTEBLocation(hThread);
}

__declspec(dllexport) bool TITCALL HideDebugger(HANDLE hProcess, DWORD PatchAPILevel)
{
    return emu.HideDebugger(hProcess, PatchAPILevel);
}

__declspec(dllexport) HANDLE TITCALL TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId)
{
    return emu.TitanOpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
}

__declspec(dllexport) HANDLE TITCALL TitanOpenThread(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwThreadId)
{
    return emu.TitanOpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
}

//Registers
__declspec(dllexport) ULONG_PTR TITCALL GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister)
{
    return emu.GetContextDataEx(hActiveThread, IndexOfRegister);
}

__declspec(dllexport) bool TITCALL SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue)
{
    return emu.SetContextDataEx(hActiveThread, IndexOfRegister, NewRegisterValue);
}

__declspec(dllexport) bool TITCALL GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    return emu.GetFullContextDataEx(hActiveThread, titcontext);
}

__declspec(dllexport) bool TITCALL SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    return emu.SetFullContextDataEx(hActiveThread, titcontext);
}

__declspec(dllexport) void TITCALL GetMMXRegisters(uint64_t mmx[8], TITAN_ENGINE_CONTEXT_t* titcontext)
{
    emu.GetMMXRegisters(mmx, titcontext);
}

__declspec(dllexport) void TITCALL Getx87FPURegisters(x87FPURegister_t x87FPURegisters[8], TITAN_ENGINE_CONTEXT_t* titcontext)
{
    emu.Getx87FPURegisters(x87FPURegisters, titcontext);
}

//PE
__declspec(dllexport) bool TITCALL StaticFileLoadW(const wchar_t* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA)
{
    return emu.StaticFileLoadW(szFileName, DesiredAccess, SimulateLoad, FileHandle, LoadedSize, FileMap, FileMapVA);
}

__declspec(dllexport) bool TITCALL StaticFileUnloadW(const wchar_t* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA)
{
    return emu.StaticFileUnloadW(szFileName, CommitChanges, FileHandle, LoadedSize, FileMap, FileMapVA);
}

__declspec(dllexport) ULONG_PTR TITCALL ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType)
{
    return emu.ConvertFileOffsetToVA(FileMapVA, AddressToConvert, ReturnType);
}

__declspec(dllexport) ULONG_PTR TITCALL ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType)
{
    return emu.ConvertVAtoFileOffsetEx(FileMapVA, FileSize, ImageBase, AddressToConvert, AddressIsRVA, ReturnType);
}

__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData)
{
    return emu.GetPE32DataFromMappedFile(FileMapVA, WhichSection, WhichData);
}

__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataW(const wchar_t* szFileName, DWORD WhichSection, DWORD WhichData)
{
    return emu.GetPE32DataW(szFileName, WhichSection, WhichData);
}

__declspec(dllexport) bool TITCALL IsFileDLLW(const wchar_t* szFileName, ULONG_PTR FileMapVA)
{
    return emu.IsFileDLLW(szFileName, FileMapVA);
}

__declspec(dllexport) bool TITCALL TLSGrabCallBackDataW(const wchar_t* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks)
{
    return emu.TLSGrabCallBackDataW(szFileName, ArrayOfCallBacks, NumberOfCallBacks);
}

//Software Breakpoints
__declspec(dllexport) bool TITCALL SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack)
{
    return emu.SetBPX(bpxAddress, bpxType, bpxCallBack);
}

__declspec(dllexport) bool TITCALL DeleteBPX(ULONG_PTR bpxAddress)
{
    return emu.DeleteBPX(bpxAddress);
}

__declspec(dllexport) bool TITCALL IsBPXEnabled(ULONG_PTR bpxAddress)
{
    return emu.IsBPXEnabled(bpxAddress);
}

__declspec(dllexport) void TITCALL SetBPXOptions(long DefaultBreakPointType)
{
    emu.SetBPXOptions(DefaultBreakPointType);
}

//Memory Breakpoints
__declspec(dllexport) bool TITCALL SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack)
{
    return emu.SetMemoryBPXEx(MemoryStart, SizeOfMemory, BreakPointType, RestoreOnHit, bpxCallBack);
}

__declspec(dllexport) bool TITCALL RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory)
{
    return emu.RemoveMemoryBPX(MemoryStart, SizeOfMemory);
}

//Hardware Breakpoints
__declspec(dllexport) bool TITCALL SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack)
{
    return emu.SetHardwareBreakPoint(bpxAddress, IndexOfRegister, bpxType, bpxSize, bpxCallBack);
}

__declspec(dllexport) bool TITCALL DeleteHardwareBreakPoint(DWORD IndexOfRegister)
{
    return emu.DeleteHardwareBreakPoint(IndexOfRegister);
}

__declspec(dllexport) bool TITCALL GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex)
{
    return emu.GetUnusedHardwareBreakPointRegister(RegisterIndex);
}

//Generic Breakpoints
__declspec(dllexport) bool TITCALL RemoveAllBreakPoints(DWORD RemoveOption)
{
    return emu.RemoveAllBreakPoints(RemoveOption);
}

//Stepping
__declspec(dllexport) void TITCALL StepOver(LPVOID traceCallBack)
{
    emu.StepOver(traceCallBack);
}

__declspec(dllexport) void TITCALL StepInto(LPVOID traceCallBack)
{
    emu.StepInto(traceCallBack);
}