#include "Emulation.h"
#include "Emulation.Instructions.h"

namespace GleeBug {

bool Emulator::IsActive() const
{
    return _active;
}

bool Emulator::WaitForEvent(DEBUG_EVENT& debugEvent) const
{
    return false;
}

bool Emulator::Emulate(Thread* thread)
{
    SetActiveThread(thread);

    return true;
}

void Emulator::Flush()
{
    if (_registers != nullptr)
    {
        delete _registers;
        _registers = nullptr;
    }
    _activeThread = nullptr;
}

void Emulator::SetActiveThread(Thread *thread)
{
    if (_activeThread != thread)
    {
        // Commit current state if we change threads between.
        Flush();

        _registers = new Registers(thread->hThread, CONTEXT_CONTROL);
        _activeThread = thread;
    }
}

}
