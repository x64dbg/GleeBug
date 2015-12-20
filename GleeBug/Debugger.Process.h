#ifndef _DEBUGGER_PROCESS_H
#define _DEBUGGER_PROCESS_H

#include "Debugger.Global.h"
#include "Debugger.Thread.h"
#include "Debugger.Dll.h"
#include "Debugger.Breakpoint.h"

namespace GleeBug
{
    /**
    \brief Process information structure.
    */
    class ProcessInfo
    {
    public:
        HANDLE hProcess;
        uint32 dwProcessId;
        uint32 dwMainThreadId;

        ThreadInfo* thread;
        bool systemBreakpoint;

        ThreadMap threads; //DO NOT COPY THESE OBJECTS!
        DllMap dlls;
        BreakpointMap breakpoints;
        SoftwareBreakpointMap softwareBreakpointReferences;
        BreakpointCallbackMap breakpointCallbacks;
        BreakpointInfo hardwareBreakpoints[4];

        /**
        \brief Constructor.
        \param hProcess Process handle.
        \param dwProcessId Identifier for the process.
        \param dwMainThreadId Identifier for the main thread.
        */
        explicit ProcessInfo(HANDLE hProcess, uint32 dwProcessId, uint32 dwMainThreadId);

        /**
        \brief Read memory from the process.
        \param address The virtual address to read from.
        \param [out] buffer Destination buffer. Cannot be null. May be filled partially on failure.
        \param size The size to read.
        \param bytesRead (Optional) Number of bytes read (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemRead(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr) const;

        /**
        \brief Safely read memory from the process, filtering out breakpoint bytes.
        \param address The virtual address to read from.
        \param [out] buffer Destination buffer. Cannot be null. May be filled partially on failure.
        \param size The size to read.
        \param bytesRead (Optional) Number of bytes read (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemReadSafe(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr) const;

        /**
        \brief Write memory to the process.
        \param address The virtual address to write to.
        \param [in] buffer Source buffer. Cannot be null.
        \param size The size to write.
        \param bytesWritten (Optional) Number of bytes written (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemWrite(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr);

        /**
        \brief Safely write memory to the process, preserving breakpoint bytes.
        \param address The virtual address to write to.
        \param [in] buffer Source buffer. Cannot be null.
        \param size The size to write.
        \param bytesWritten (Optional) Number of bytes written (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemWriteSafe(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr);

        /**
        \brief Check if an address is a valid read pointer.
        \param address The address to check.
        \return true if the address is valid, false otherwise.
        */
        bool MemIsValidPtr(ptr address) const;

        /**
        \brief Sets a software breakpoint.
        \param address The address to set the breakpoint on.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \param type (Optional) The software breakpoint type.
        \return true if the breakpoint was set, false otherwise.
        */
        bool SetBreakpoint(ptr address, bool singleshoot = false, SoftwareBreakpointType type = SoftwareBreakpointType::ShortInt3);

        /**
        \brief Sets a software breakpoint.
        \param address The address to set the breakpoint on.
        \param cbBreakpoint The breakpoint callback. Can be written using BIND1(this, MyDebugger::cb).
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \param type (Optional) The software breakpoint type.
        \return true if the breakpoint was set, false otherwise.
        */
        bool SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshoot = false, SoftwareBreakpointType type = SoftwareBreakpointType::ShortInt3);

        /**
        \brief Sets a software breakpoint.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param address The address to set the breakpoint on.
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \param type (Optional) The software breakpoint type.
        \return true if the breakpoint was set, false otherwise.
        */
        template <typename T>
        bool SetBreakpoint(ptr address, T* debugger, void(T::*callback)(const BreakpointInfo & info), bool singleshoot = false, SoftwareBreakpointType type = SoftwareBreakpointType::ShortInt3)
        {
            static_cast<void>(static_cast<Debugger*>(debugger));
            return SetBreakpoint(address, std::bind(callback, debugger, std::placeholders::_1), singleshoot, type);
        }

        /**
        \brief Deletes a software breakpoint.
        \param address The address to delete the breakpoint from.
        \return true if the breakpoint was deleted, false otherwise.
        */
        bool DeleteBreakpoint(ptr address);

        /**
        \brief Attempts to find a free hardware breakpoint slot.
        \param [out] slot First free slot found, has no meaning when the function fails.
        \return true if a free slot was found, false otherwise.
        */
        bool GetFreeHardwareBreakpointSlot(HardwareBreakpointSlot & slot) const;

        /**
        \brief Sets a hardware breakpoint.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use ProcessInfo::GetFreeHardwareBreakpointSlot.
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        bool SetHardwareBreakpoint(ptr address, HardwareBreakpointSlot slot, HardwareBreakpointType type = HardwareBreakpointType::Execute, HardwareBreakpointSize size = HardwareBreakpointSize::SizeByte, bool singleshoot = false);

        /**
        \brief Sets a hardware breakpoint.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use ProcessInfo::GetFreeHardwareBreakpointSlot.
        \param cbBreakpoint The breakpoint callback. Can be written using BIND1(this, MyDebugger::cb).
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        bool SetHardwareBreakpoint(ptr address, HardwareBreakpointSlot slot, const BreakpointCallback & cbBreakpoint, HardwareBreakpointType type = HardwareBreakpointType::Execute, HardwareBreakpointSize size = HardwareBreakpointSize::SizeByte, bool singleshoot = false);

        /**
        \brief Sets a hardware breakpoint.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use ProcessInfo::GetFreeHardwareBreakpointSlot.
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        template <typename T>
        bool SetHardwareBreakpoint(ptr address, HardwareBreakpointSlot slot, T* debugger, void(T::*callback)(const BreakpointInfo & info), HardwareBreakpointType type = HardwareBreakpointType::Execute, HardwareBreakpointSize size = HardwareBreakpointSize::SizeByte, bool singleshoot = false)
        {
            static_cast<void>(static_cast<Debugger*>(debugger));
            return SetHardwareBreakpoint(address, slot, std::bind(callback, debugger, std::placeholders::_1), type, size, singleshoot);
        }

        /**
        \brief Deletes a hardware breakpoint.
        \param address The address the hardware breakpoint is set on.
        \return true if the hardware breakpoint was deleted, false otherwise.
        */
        bool DeleteHardwareBreakpoint(ptr address);

        /**
        \brief Deletes a breakpoint.
        \param info The breakpoint information.
        \return true if the breakpoint was deleted, false otherwise.
        */
        bool DeleteGenericBreakpoint(const BreakpointInfo & info);
    };
};

#endif //_DEBUGGER_PROCESS_H