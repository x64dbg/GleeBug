#ifndef DEBUGGER_PROCESS_H
#define DEBUGGER_PROCESS_H

#include "Debugger.Global.h"
#include "Debugger.Thread.h"
#include "Debugger.Dll.h"
#include "Debugger.Breakpoint.h"
#include "Static.Pattern.h"
#include <capstone_wrapper/capstone_wrapper.h>

namespace GleeBug
{
    /**
    \brief Process information structure.
    */
    class Process
    {
    public:
        HANDLE hProcess;
        uint32 dwProcessId;
        uint32 dwMainThreadId;
        CREATE_PROCESS_DEBUG_INFO createProcessInfo; //hFile is invalid, possibly other handles too!

        Thread* thread;
        bool systemBreakpoint;
        bool permanentDep;

        ThreadMap threads; //DO NOT COPY THESE OBJECTS!
        DllMap dlls;
        BreakpointMap breakpoints;
        SoftwareBreakpointMap softwareBreakpointReferences;
        BreakpointCallbackMap breakpointCallbacks;
        BreakpointInfo hardwareBreakpoints[4];
        MemoryBreakpointSet memoryBreakpointRanges;
        MemoryBreakpointMap memoryBreakpointPages;

        /**
        \brief Constructor.
        \param hProcess Process handle.
        \param dwProcessId Identifier for the process.
        \param dwMainThreadId Identifier for the main thread.
        \param createProcessInfo The process creation info.
        */
        explicit Process(HANDLE hProcess, uint32 dwProcessId, uint32 dwMainThreadId, const CREATE_PROCESS_DEBUG_INFO & createProcessInfo);

        /**
        \brief Read memory from the process.
        \param address The virtual address to read from.
        \param [out] buffer Destination buffer. Cannot be null. May be filled partially on failure.
        \param size The size to read.
        \param bytesRead (Optional) Number of bytes read (should be equal to size on success).
        \param safe Whether to call MemReadSafe or MemReadUnsafe.
        \return true if it succeeds, false if it fails.
        */
        bool MemRead(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr, bool safe = true) const
        {
            if (safe)
                return MemReadSafe(address, buffer, size, bytesRead);
            return MemRead(address, buffer, size, bytesRead);
        }

        /**
        \brief Read memory from the process. This function should be used for internal reasons only!
        \param address The virtual address to read from.
        \param [out] buffer Destination buffer. Cannot be null. May be filled partially on failure.
        \param size The size to read.
        \param bytesRead (Optional) Number of bytes read (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemReadUnsafe(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr) const;

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
        \param safe Wheter to call MemWriteSafe or MemWriteUnsafe.
        \return true if it succeeds, false if it fails.
        */
        bool MemWrite(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr, bool safe = true)
        {
            if (safe)
                return MemWriteSafe(address, buffer, size, bytesWritten);
            return MemWriteUnsafe(address, buffer, size, bytesWritten);
        }

        /**
        \brief Write memory to the process. This function should be used for internal reasons only!
        \param address The virtual address to write to.
        \param [in] buffer Source buffer. Cannot be null.
        \param size The size to write.
        \param bytesWritten (Optional) Number of bytes written (should be equal to size on success).
        \return true if it succeeds, false if it fails.
        */
        bool MemWriteUnsafe(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr);

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
        \brief Memory protect (execute VirtualProtect in the context of the process).
        \param address The address to change protection for.
        \param size The size to change protection for.
        \param newProtect The new protection.
        \param [out] oldProtect The old protection.
        \return true if it succeeds, false if it fails.
        */
        bool MemProtect(ptr address, ptr size, DWORD newProtect, DWORD* oldProtect = nullptr);

        /**
        \brief Finds the first occurrence of a pattern in process memory.
        \param data The address to start searching from.
        \param datasize The size to search in.
        \param pattern The pattern to find.
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return Memory address when found, 0 when not found.
        */
        ptr MemFindPattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe = true) const;

        /**
        \brief Finds the first occurrence of a pattern in process memory.
        \param data The address to start searching from.
        \param datasize The size to search in.
        \param pattern The pattern to find.
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return Memory address when found, 0 when not found.
        */
        ptr MemFindPattern(ptr data, size_t datasize, const std::string & pattern, bool safe = true) const
        {
            return MemFindPattern(data, datasize, Pattern::Transform(pattern), safe);
        }

        /**
        \brief Finds the first occurrence of a pattern in process memory.
        \param data The address to start searching from.
        \param datasize The size to search in.
        \param pattern The pattern to find.
        \param patternsize The size of the pattern to find.
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return Memory address when found, 0 when not found.
        */
        ptr MemFindPattern(ptr data, size_t datasize, const uint8* pattern, size_t patternsize, bool safe = true) const;

        /**
        \brief Writes a pattern in process memory. This function writes as many bytes as possible from the pattern.
        \param [in,out] data The address to write the pattern in.
        \param datasize The size to write in.
        \param pattern Specifies the pattern.
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return true if it succeeds, false if it fails.
        */
        bool MemWritePattern(ptr data, size_t datasize, const Pattern::WildcardPattern & pattern, bool safe = true);

        /**
        \brief Writes a pattern in process memory. This function writes as many bytes as possible from the pattern.
        \param [in,out] data The address to write the pattern in.
        \param datasize The size to write in.
        \param pattern Specifies the pattern. The pattern supports wildcards (1? ?? ?6 78).
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return true if it succeeds, false if it fails.
        */
        bool MemWritePattern(ptr data, size_t datasize, const std::string & pattern, bool safe = true)
        {
            return MemWritePattern(data, datasize, Pattern::Transform(pattern), safe);
        }

        /**
        \brief Search and replace a pattern in process memory.
        \param [in,out] data The address to search and replace in.
        \param datasize The size to search and replace in.
        \param searchpattern The pattern to find.
        \param replacepattern The pattern to replace the found occurrence with.
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return true if it succeeds, false if it fails.
        */
        bool MemSearchAndReplace(ptr data, size_t datasize, const Pattern::WildcardPattern & searchpattern, const Pattern::WildcardPattern & replacepattern, bool safe = true);

        /**
        \brief Search and replace a pattern in process memory.
        \param [in,out] data The address to search and replace in.
        \param datasize The size to search and replace in.
        \param searchpattern The pattern to find. The pattern supports wildcards (1? ?? ?6 78).
        \param replacepattern The pattern to replace the found occurrence with. The pattern supports wildcards (1? ?? ?6 78).
        \param safe Use the safe memory functions (eg do not consider software breakpoint data).
        \return true if it succeeds, false if it fails.
        */
        bool MemSearchAndReplace(ptr data, size_t datasize, const std::string & searchpattern, const std::string & replacepattern, bool safe = true)
        {
            return MemSearchAndReplace(data, datasize, Pattern::Transform(searchpattern), Pattern::Transform(replacepattern), safe);
        }

        /**
        \brief Sets a software breakpoint.
        \param address The address to set the breakpoint on.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \param type (Optional) The software breakpoint type.
        \return true if the breakpoint was set, false otherwise.
        */
        bool SetBreakpoint(ptr address, bool singleshoot = false, SoftwareType type = SoftwareType::ShortInt3);

        /**
        \brief Sets a software breakpoint.
        \param address The address to set the breakpoint on.
        \param cbBreakpoint The breakpoint callback. Can be written using BIND1(this, MyDebugger::cb).
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \param type (Optional) The software breakpoint type.
        \return true if the breakpoint was set, false otherwise.
        */
        bool SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshoot = false, SoftwareType type = SoftwareType::ShortInt3);

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
        bool SetBreakpoint(ptr address, T* debugger, void(T::*callback)(const BreakpointInfo & info), bool singleshoot = false, SoftwareType type = SoftwareType::ShortInt3)
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
        bool GetFreeHardwareBreakpointSlot(HardwareSlot & slot) const;

        /**
        \brief Sets a hardware breakpoint.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use Process::GetFreeHardwareBreakpointSlot.
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::SizeByte, bool singleshoot = false);

        /**
        \brief Sets a hardware breakpoint.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use Process::GetFreeHardwareBreakpointSlot.
        \param cbBreakpoint The breakpoint callback. Can be written using BIND1(this, MyDebugger::cb).
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::SizeByte, bool singleshoot = false);

        /**
        \brief Sets a hardware breakpoint.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param address The address to set the hardware breakpoint on.
        \param slot The hardware breakpoint register slot. Use Process::GetFreeHardwareBreakpointSlot.
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        \param type (Optional) The hardware breakpoint type.
        \param size (Optional) The hardware breakpoint size.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the hardware breakpoint was set, false otherwise.
        */
        template <typename T>
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, T* debugger, void(T::*callback)(const BreakpointInfo & info), HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::SizeByte, bool singleshoot = false)
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
        \brief Sets new page protection to trigger an exception for certain memory breakpoint types.
        \param page The page address.
        \param data The current protection of the page.
        \param type The memory breakpoint type to trigger an exception for.
        \return true if it succeeds, false if it fails.
        */
        bool SetNewPageProtection(ptr page, MemoryBreakpointData & data, MemoryType type);

        /**
        \brief Sets a memory breakpoint.
        \param address The address to set the memory breakpoint on.
        \param size Size of the memory breakpoint (in bytes).
        \param type (Optional) The memory breakpoint type.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the memory breakpoint was set, false otherwise.
        */
        bool SetMemoryBreakpoint(ptr address, ptr size, MemoryType type = MemoryType::Access, bool singleshoot = true);

        /**
        \brief Sets a memory breakpoint.
        \param address The address to set the memory breakpoint on.
        \param size Size of the memory breakpoint (in bytes).
        \param cbBreakpoint The breakpoint callback. Can be written using BIND1(this, MyDebugger::cb).
        \param type (Optional) The memory breakpoint type.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the memory breakpoint was set, false otherwise.
        */
        bool SetMemoryBreakpoint(ptr address, ptr size, const BreakpointCallback & cbBreakpoint, MemoryType type = MemoryType::Access, bool singleshoot = true);

        /**
        \brief Sets a hardware breakpoint.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param address The address to set the hardware breakpoint on.
        \param size Size of the memory breakpoint (in bytes).
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        \param type (Optional) The memory breakpoint type.
        \param singleshoot (Optional) True to remove the breakpoint after the first hit.
        \return true if the memory breakpoint was set, false otherwise.
        */
        template <typename T>
        bool SetMemoryBreakpoint(ptr address, ptr size, T* debugger, void(T::*callback)(const BreakpointInfo & info), MemoryType type = MemoryType::Access, bool singleshoot = true)
        {
            static_cast<void>(static_cast<Debugger*>(debugger));
            return SetMemoryBreakpoint(address, size, std::bind(callback, debugger, std::placeholders::_1), type, singleshoot);
        }

        /**
        \brief Deletes a hardware breakpoint.
        \param address The address the hardware breakpoint is set on.
        \return true if the hardware breakpoint was deleted, false otherwise.
        */
        bool DeleteMemoryBreakpoint(ptr address);

        /**
        \brief Deletes a breakpoint.
        \param info The breakpoint information.
        \return true if the breakpoint was deleted, false otherwise.
        */
        bool DeleteGenericBreakpoint(const BreakpointInfo & info);

        /**
        \brief Step over.
        \param cbStep Step callback. Can be written using BIND(this, MyDebugger::cb).
        */
        void StepOver(const StepCallback & cbStep);

        /**
        \brief Step over.
        \tparam T Generic type parameter. Must be a subclass of Debugger.
        \param debugger This pointer to a subclass of Debugger.
        \param callback Pointer to the callback. Written like: &MyDebugger::cb
        */
        template<typename T>
        void StepOver(T* debugger, void(T::*callback)())
        {
            static_cast<void>(static_cast<Debugger*>(debugger));
            StepOver(std::bind(callback, debugger));
        }

        bool RegReadContext()
        {
            auto result = true;
            for(auto & thread : this->threads)
                if(!thread.second.RegReadContext())
                    result = false;
            return result;
        }

        bool RegWriteContext()
        {
            auto result = true;
            for(auto & thread : this->threads)
                if(!thread.second.RegWriteContext())
                    result = false;
            return result;
        }

        private:
            Capstone mCapstone;
    };
};

#endif //DEBUGGER_PROCESS_H