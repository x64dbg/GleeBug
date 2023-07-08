#ifndef DEBUGGER_THREAD_H
#define DEBUGGER_THREAD_H

#include "Debugger.Global.h"
#include "Debugger.Breakpoint.h"

namespace GleeBug
{
    /**
    \brief Thread information structure.
    */
    class Thread
    {
    public:
        HANDLE hThread;
        uint32 dwThreadId;
        ptr lpThreadLocalBase;
        ptr lpStartAddress;

        StepCallbackVector stepCallbacks;
        bool isSingleStepping;
        bool isInternalStepping;
        StepCallback cbInternalStep;

        /**
        \brief Constructor.
        \param hThread Thread handle.
        \param dwThreadId Identifier for the thread.
        \param lpThreadLocalBase The thread local base.
        \param lpStartAddress The start address.
        */
        explicit Thread(HANDLE hThread, uint32 dwThreadId, LPVOID lpThreadLocalBase, LPVOID lpStartAddress);

        /**
        \brief Copy constructor.
        */
        Thread(const Thread & other) = delete;

        /**
        \brief Step into.
        */
        void StepInto();

        /**
        \brief Step into.
        \param cbStep Step callback. Can be written using BIND(this, MyDebugger::cb).
        */
        void StepInto(const StepCallback & cbStep);

        /**
        \brief Step into.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        */
        template<typename T>
        void StepInto(T* debugger, void(T::*callback)())
        {
            static_cast<void>(static_cast<Debugger*>(debugger));
            StepInto(std::bind(callback, debugger));
        }

        /**
        \brief Sets a hardware breakpoint.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use Process::GetFreeHardwareBreakpointSlot.
        \param type The hardware breakpoint type.
        \param size The hardware breakpoint size.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type, HardwareSize size);

        /**
        \brief Deletes a hardware breakpoint.
        \param slot The slot to remove the hardware breakpoint from.
        \return true if the hardware breakpoint was deleted, false otherwise.
        */
        bool DeleteHardwareBreakpoint(HardwareSlot slot);

        /**
        \brief Suspends this thread.
        \return true if it succeeds, false if it fails.
        */
        bool Suspend();

        /**
        \brief Resumes this thread.
        \return true if it succeeds, false if it fails.
        */
        bool Resume();
    };
};

#endif //DEBUGGER_THREADS_H