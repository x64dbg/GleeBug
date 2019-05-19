#ifndef EMULATION_H
#define EMULATION_H

#include "Debugger.Global.h"
#include "Debugger.Breakpoint.h"
#include "Debugger.Thread.h"
#include "Debugger.Thread.Registers.h"

#include <deque>

namespace GleeBug
{

class Process;

class X86Emulator
{
    bool _active = false;
    Thread *_activeThread = nullptr;
    Registers* _registers = nullptr;
    Process* _process = nullptr;
    bool _hasEvent = false;
    DEBUG_EVENT _currentEvent{};

public:
    X86Emulator(Process* process);

    bool IsActive() const;
    bool WaitForEvent(DEBUG_EVENT& debugEvent);
    bool Emulate(Thread* thread);
    void Flush();

private:
    void SetActiveThread(Thread *thread);
};

} // GleeBug

#endif // EMULATION_H
