#include "Debugger.h"

namespace GleeBug
{
    void Debugger::createProcessEvent(const CREATE_PROCESS_DEBUG_INFO & createProcess)
    {
        //process housekeeping
        _processes.insert({ _debugEvent.dwProcessId,
            ProcessInfo(_debugEvent.dwProcessId,
            createProcess.hProcess,
            _debugEvent.dwThreadId) });
        _process = &_processes.find(_debugEvent.dwProcessId)->second;

        //thread housekeeping (main thread is created implicitly)
        _process->threads.insert({ _debugEvent.dwThreadId,
            ThreadInfo(_debugEvent.dwThreadId,
            createProcess.hThread,
            createProcess.lpThreadLocalBase,
            createProcess.lpStartAddress) });
        _thread = _process->thread = &_process->threads.find(_debugEvent.dwThreadId)->second;
        _registers = &_thread->registers;

        //read thread context from main thread
        if (!_thread->RegReadContext())
            cbInternalError("ThreadInfo::RegReadContext() failed!");

        //call the debug event callback
        cbCreateProcessEvent(createProcess, *_process);

        //close the file handle
        CloseHandle(createProcess.hFile);
    }

    void Debugger::exitProcessEvent(const EXIT_PROCESS_DEBUG_INFO & exitProcess)
    {
        //check if the terminated process is the main debuggee
        if (_debugEvent.dwProcessId == _mainProcess.dwProcessId)
            _breakDebugger = true;

        //call the debug event callback
        cbExitProcessEvent(exitProcess, *_process);

        //process housekeeping
        _processes.erase(_debugEvent.dwProcessId);

        //set the current process
        _process = nullptr;
        _thread = nullptr;
        _registers = nullptr;
    }
};