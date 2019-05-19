#ifndef EMULATION_H
#define EMULATION_H

#include "Debugger.Global.h"
#include "Debugger.Breakpoint.h"
#include "Debugger.Thread.h"
#include "Debugger.Thread.Registers.h"

namespace GleeBug
{

class Process;

class Emulator
{
    bool _active = false;
    Thread *_activeThread = nullptr;
    Registers* _registers = nullptr;

public:
    bool IsActive() const;
    bool WaitForEvent(DEBUG_EVENT& debugEvent) const;
    bool Emulate(Thread* thread);
    void Flush();

private:
    void SetActiveThread(Thread *thread);
};

} // GleeBug

#endif // EMULATION_H
