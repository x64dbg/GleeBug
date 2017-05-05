#ifndef DEBUGGER_THREAD_REGISTERS_H
#define DEBUGGER_THREAD_REGISTERS_H

#include "Debugger.Global.h"

namespace GleeBug
{
    /**
    \brief Thread register context.
    */
    class Registers
    {
        friend class Register;
        friend class Thread;

    public:
        /**
        \brief Default constructor.
        */
        Registers();

#include "Debugger.Thread.Registers.Register.h"

        Register<R::DR0, ptr> Dr0;
        Register<R::DR1, ptr> Dr1;
        Register<R::DR2, ptr> Dr2;
        Register<R::DR3, ptr> Dr3;
        Register<R::DR6, ptr> Dr6;
        Register<R::DR7, ptr> Dr7;

        Register<R::EFlags, uint32> Eflags;

        Register<R::EAX, uint32> Eax;
        Register<R::AX, uint16> Ax;
        Register<R::AH, uint8> Ah;
        Register<R::AL, uint8> Al;
        Register<R::EBX, uint32> Ebx;
        Register<R::BX, uint16> Bx;
        Register<R::BH, uint8> Bh;
        Register<R::BL, uint8> Bl;
        Register<R::ECX, uint32> Ecx;
        Register<R::CX, uint16> Cx;
        Register<R::CH, uint8> Ch;
        Register<R::CL, uint8> Cl;
        Register<R::EDX, uint32> Edx;
        Register<R::DX, uint16> Dx;
        Register<R::DH, uint8> Dh;
        Register<R::DL, uint8> Dl;
        Register<R::EDI, uint32> Edi;
        Register<R::DI, uint16> Di;
        Register<R::ESI, uint32> Esi;
        Register<R::SI, uint16> Si;
        Register<R::EBP, uint32> Ebp;
        Register<R::BP, uint16> Bp;
        Register<R::ESP, uint32> Esp;
        Register<R::SP, uint16> Sp;
        Register<R::EIP, uint32> Eip;

#ifdef _WIN64
        Register<R::RAX, uint64> Rax;
        Register<R::RBX, uint64> Rbx;
        Register<R::RCX, uint64> Rcx;
        Register<R::RDX, uint64> Rdx;
        Register<R::RSI, uint64> Rsi;
        Register<R::SIL, uint8> Sil;
        Register<R::RDI, uint64> Rdi;
        Register<R::DIL, uint8> Dil;
        Register<R::RBP, uint64> Rbp;
        Register<R::BPL, uint8> Bpl;
        Register<R::RSP, uint64> Rsp;
        Register<R::SPL, uint8> Spl;
        Register<R::RIP, uint64> Rip;
        Register<R::R8, uint64> R8;
        Register<R::R8D, uint32> R8d;
        Register<R::R8W, uint16> R8w;
        Register<R::R8B, uint8> R8b;
        Register<R::R9, uint64> R9;
        Register<R::R9D, uint32> R9d;
        Register<R::R9W, uint16> R9w;
        Register<R::R9B, uint8> R9b;
        Register<R::R10, uint64> R10;
        Register<R::R10D, uint32> R10d;
        Register<R::R10W, uint16> R10w;
        Register<R::R10B, uint8> R10b;
        Register<R::R11, uint64> R11;
        Register<R::R11D, uint32> R11d;
        Register<R::R11W, uint16> R11w;
        Register<R::R11B, uint8> R11b;
        Register<R::R12, uint64> R12;
        Register<R::R12D, uint32> R12d;
        Register<R::R12W, uint16> R12w;
        Register<R::R12B, uint8> R12b;
        Register<R::R13, uint64> R13;
        Register<R::R13D, uint32> R13d;
        Register<R::R13W, uint16> R13w;
        Register<R::R13B, uint8> R13b;
        Register<R::R14, uint64> R14;
        Register<R::R14D, uint32> R14d;
        Register<R::R14W, uint16> R14w;
        Register<R::R14B, uint8> R14b;
        Register<R::R15, uint64> R15;
        Register<R::R15D, uint32> R15d;
        Register<R::R15W, uint16> R15w;
        Register<R::R15B, uint8> R15b;
#endif //_WIN64

        Register<R::GAX, ptr> Gax;
        Register<R::GBX, ptr> Gbx;
        Register<R::GCX, ptr> Gcx;
        Register<R::GDX, ptr> Gdx;
        Register<R::GDI, ptr> Gdi;
        Register<R::GSI, ptr> Gsi;
        Register<R::GBP, ptr> Gbp;
        Register<R::GSP, ptr> Gsp;
        Register<R::GIP, ptr> Gip;

#include "Debugger.Thread.Registers.Flag.h"

        Flag<F::Trap> TrapFlag;
        Flag<F::Resume> ResumeFlag;

        /**
        \brief Gets the given register.
        \param reg The register to get.
        \return The register value.
        */
        ptr Get(R reg);

        /**
        \brief Sets a given register.
        \param reg The register to set.
        \param value The new register value.
        */
        void Set(R reg, ptr value);

        /**
        \brief Gets a flag.
        \param flag The flag to get.
        \return true if the flag is set, false otherwise.
        */
        bool GetFlag(F flag);

        /**
        \brief Sets a flag.
        \param set (Optional) true to set the flag, false to unset the flag.
        */
        void SetFlag(F flag, bool set = true);

        /**
        \brief Gets a pointer to the context object.
        \return This function will never return a nullptr.
        */
        const CONTEXT* GetContext();

        /**
        \brief Sets the CONTEXT.
        \param context The context to set.
        */
        void SetContext(const CONTEXT & context);

    private:
        CONTEXT mContext;

        LPCONTEXT mLazyOldContext = nullptr;
        HANDLE mLazyThread = nullptr;
        bool mLazySet = false;

        /**
        \brief Lazily set CONTEXT. This will only actually retrieve the context if a function in this thread is called.
        \param oldContext Pointer to the old context, used to determine if updates are required.
        \param hThread Handle of the thread to get the context from if required.
        */
        void setContextLazy(CONTEXT* oldContext, HANDLE hThread);

        /**
        \brief Retrieve the actual context if setContextLazy has been called.
        \return Whether retrieving the actual context was successful.
        */
        bool handleLazyContext();

        void* getPtr(R reg);
    };
};

#endif //DEBUGGER_THREAD_REGISTERS_H