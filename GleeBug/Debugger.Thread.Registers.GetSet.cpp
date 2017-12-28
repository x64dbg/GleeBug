#include "Debugger.Thread.Registers.h"

#define REGPTR(x) (void*)(&x)
#define REGPTRN(x, n) (void*)((char*)&x + n)

#ifdef _WIN64
#define contextGax mContext.Rax
#define contextGbx mContext.Rbx
#define contextGcx mContext.Rcx
#define contextGdx mContext.Rdx
#define contextGdi mContext.Rdi
#define contextGsi mContext.Rsi
#define contextGbp mContext.Rbp
#define contextGsp mContext.Rsp
#define contextGip mContext.Rip
#else //x32
#define contextGax mContext.Eax
#define contextGbx mContext.Ebx
#define contextGcx mContext.Ecx
#define contextGdx mContext.Edx
#define contextGdi mContext.Edi
#define contextGsi mContext.Esi
#define contextGbp mContext.Ebp
#define contextGsp mContext.Esp
#define contextGip mContext.Eip
#endif //_WIN64

#ifdef _WIN64
#define uint32_lo(x) ptr(x & 0xFFFFFFFF)
#else //x32
#define uint32_lo(x) ptr(x)
#endif //_WIN64
#define uint16_lo(x) ptr(x & 0xFFFF)
#define uint8_hi(x) ptr((x >> 8) & 0xFF)
#define uint8_lo(x) ptr(x & 0xFF)

#define PTR_uint32_lo(x) REGPTR(x)
#define PTR_uint16_lo(x) REGPTR(x)
#define PTR_uint8_lo(x) REGPTR(x)
#define PTR_uint16_hi(x) REGPTRN(x, 2)
#define PTR_uint8_hi(x) REGPTRN(x, 1)

#ifdef _WIN64
#define set_uint32_lo(x, y) x = (x & ~0xFFFFFFFF) | uint32_lo(y)
#else //x32
#define set_uint32_lo(x, y) x = y
#endif //_WIN64
#define set_uint16_lo(x, y) x = (x & ~0xFFFF) | uint16_lo(y)
#define set_uint8_hi(x, y) x = (x & ~0xFF00) | (uint8_lo(y) << 8)
#define set_uint8_lo(x, y) x = (x & ~0xFF) | uint8_lo(y)

#define TRAP_FLAG 0x100
#define RESUME_FLAG 0x10000

namespace GleeBug
{
    ptr Registers::Get(R reg)
    {
        handleLazyContext();

        switch (reg)
        {
        case R::DR0:
            return ptr(mContext.Dr0);
        case R::DR1:
            return ptr(mContext.Dr1);
        case R::DR2:
            return ptr(mContext.Dr2);
        case R::DR3:
            return ptr(mContext.Dr3);
        case R::DR6:
            return ptr(mContext.Dr6);
        case R::DR7:
            return ptr(mContext.Dr7);

        case R::EFlags:
            return ptr(mContext.EFlags);

        case R::EAX:
            return uint32_lo(contextGax);
        case R::AX:
            return uint16_lo(contextGax);
        case R::AH:
            return uint8_hi(contextGax);
        case R::AL:
            return uint8_lo(contextGax);
        case R::EBX:
            return uint32_lo(contextGbx);
        case R::BX:
            return uint16_lo(contextGbx);
        case R::BH:
            return uint8_hi(contextGbx);
        case R::BL:
            return uint8_lo(contextGbx);
        case R::ECX:
            return uint32_lo(contextGcx);
        case R::CX:
            return uint16_lo(contextGcx);
        case R::CH:
            return uint8_hi(contextGcx);
        case R::CL:
            return uint8_lo(contextGcx);
        case R::EDX:
            return uint32_lo(contextGdx);
        case R::DX:
            return uint16_lo(contextGdx);
        case R::DH:
            return uint8_hi(contextGdx);
        case R::DL:
            return uint8_lo(contextGdx);
        case R::EDI:
            return uint32_lo(contextGdi);
        case R::DI:
            return uint16_lo(contextGdi);
        case R::ESI:
            return uint32_lo(contextGsi);
        case R::SI:
            return uint16_lo(contextGsi);
        case R::EBP:
            return uint32_lo(contextGbp);
        case R::BP:
            return uint16_lo(contextGbp);
        case R::ESP:
            return uint32_lo(contextGsp);
        case R::SP:
            return uint16_lo(contextGsp);
        case R::EIP:
            return uint32_lo(contextGip);

#ifdef _WIN64
        case R::RAX:
            return ptr(mContext.Rax);
        case R::RBX:
            return ptr(mContext.Rbx);
        case R::RCX:
            return ptr(mContext.Rcx);
        case R::RDX:
            return ptr(mContext.Rdx);
        case R::RSI:
            return ptr(mContext.Rsi);
        case R::SIL:
            return uint8_lo(mContext.Rsi);
        case R::RDI:
            return ptr(mContext.Rdi);
        case R::DIL:
            return uint8_lo(mContext.Rdi);
        case R::RBP:
            return ptr(mContext.Rbp);
        case R::BPL:
            return uint8_lo(mContext.Rbp);
        case R::RSP:
            return ptr(mContext.Rsp);
        case R::SPL:
            return uint8_lo(mContext.Rsp);
        case R::RIP:
            return ptr(mContext.Rip);
        case R::R8:
            return ptr(mContext.R8);
        case R::R8D:
            return uint32_lo(mContext.R8);
        case R::R8W:
            return uint16_lo(mContext.R8);
        case R::R8B:
            return uint8_lo(mContext.R8);
        case R::R9:
            return ptr(mContext.R9);
        case R::R9D:
            return uint32_lo(mContext.R9);
        case R::R9W:
            return uint16_lo(mContext.R9);
        case R::R9B:
            return uint8_lo(mContext.R9);
        case R::R10:
            return ptr(mContext.R10);
        case R::R10D:
            return uint32_lo(mContext.R10);
        case R::R10W:
            return uint16_lo(mContext.R10);
        case R::R10B:
            return uint8_lo(mContext.R10);
        case R::R11:
            return ptr(mContext.R11);
        case R::R11D:
            return uint32_lo(mContext.R11);
        case R::R11W:
            return uint16_lo(mContext.R11);
        case R::R11B:
            return uint8_lo(mContext.R11);
        case R::R12:
            return ptr(mContext.R12);
        case R::R12D:
            return uint32_lo(mContext.R12);
        case R::R12W:
            return uint16_lo(mContext.R12);
        case R::R12B:
            return uint8_lo(mContext.R12);
        case R::R13:
            return ptr(mContext.R13);
        case R::R13D:
            return uint32_lo(mContext.R13);
        case R::R13W:
            return uint16_lo(mContext.R13);
        case R::R13B:
            return uint8_lo(mContext.R13);
        case R::R14:
            return ptr(mContext.R14);
        case R::R14D:
            return uint32_lo(mContext.R14);
        case R::R14W:
            return uint16_lo(mContext.R14);
        case R::R14B:
            return uint8_lo(mContext.R14);
        case R::R15:
            return ptr(mContext.R15);
        case R::R15D:
            return uint32_lo(mContext.R15);
        case R::R15W:
            return uint16_lo(mContext.R15);
        case R::R15B:
            return uint8_lo(mContext.R15);
#endif //_WIN64

        case R::GAX:
            return ptr(contextGax);
        case R::GBX:
            return ptr(contextGbx);
        case R::GCX:
            return ptr(contextGcx);
        case R::GDX:
            return ptr(contextGdx);
        case R::GDI:
            return ptr(contextGdi);
        case R::GSI:
            return ptr(contextGsi);
        case R::GBP:
            return ptr(contextGbp);
        case R::GSP:
            return ptr(contextGsp);
        case R::GIP:
            return ptr(contextGip);

        case R::GS:
            return ptr(mContext.SegGs);
        case R::FS:
            return ptr(mContext.SegFs);
        case R::ES:
            return ptr(mContext.SegEs);
        case R::DS:
            return ptr(mContext.SegDs);
        case R::CS:
            return ptr(mContext.SegCs);
        case R::SS:
            return ptr(mContext.SegSs);

        default:
            __debugbreak();
            return 0;
        }
    }

    void Registers::Set(R reg, ptr value)
    {
        handleLazyContext();

        switch (reg)
        {
        case R::DR0:
            mContext.Dr0 = value;
            break;
        case R::DR1:
            mContext.Dr1 = value;
            break;
        case R::DR2:
            mContext.Dr2 = value;
            break;
        case R::DR3:
            mContext.Dr3 = value;
            break;
        case R::DR6:
            mContext.Dr6 = value;
            break;
        case R::DR7:
            mContext.Dr7 = value;
            break;

        case R::EFlags:
            mContext.EFlags = uint32(value);
            break;

        case R::EAX:
            set_uint32_lo(contextGax, value);
            break;
        case R::AX:
            set_uint16_lo(contextGax, value);
            break;
        case R::AH:
            set_uint8_hi(contextGax, value);
            break;
        case R::AL:
            set_uint8_lo(contextGax, value);
            break;
        case R::EBX:
            set_uint32_lo(contextGbx, value);
            break;
        case R::BX:
            set_uint16_lo(contextGbx, value);
            break;
        case R::BH:
            set_uint8_hi(contextGbx, value);
            break;
        case R::BL:
            set_uint8_lo(contextGbx, value);
            break;
        case R::ECX:
            set_uint32_lo(contextGcx, value);
            break;
        case R::CX:
            set_uint16_lo(contextGcx, value);
            break;
        case R::CH:
            set_uint8_hi(contextGcx, value);
            break;
        case R::CL:
            set_uint8_lo(contextGcx, value);
            break;
        case R::EDX:
            set_uint32_lo(contextGdx, value);
            break;
        case R::DX:
            set_uint16_lo(contextGdx, value);
            break;
        case R::DH:
            set_uint8_hi(contextGdx, value);
            break;
        case R::DL:
            set_uint8_lo(contextGdx, value);
            break;
        case R::EDI:
            set_uint32_lo(contextGdi, value);
            break;
        case R::DI:
            set_uint16_lo(contextGdi, value);
            break;
        case R::ESI:
            set_uint32_lo(contextGsi, value);
            break;
        case R::SI:
            set_uint16_lo(contextGsi, value);
            break;
        case R::EBP:
            set_uint32_lo(contextGbp, value);
            break;
        case R::BP:
            set_uint16_lo(contextGbp, value);
            break;
        case R::ESP:
            set_uint32_lo(contextGsp, value);
            break;
        case R::SP:
            set_uint16_lo(contextGsp, value);
            break;
        case R::EIP:
            set_uint32_lo(contextGip, value);
            break;

#ifdef _WIN64
        case R::RAX:
            mContext.Rax = value;
            break;
        case R::RBX:
            mContext.Rbx = value;
            break;
        case R::RCX:
            mContext.Rcx = value;
            break;
        case R::RDX:
            mContext.Rdx = value;
            break;
        case R::RSI:
            mContext.Rsi = value;
            break;
        case R::SIL:
            set_uint8_lo(mContext.Rsi, value);
            break;
        case R::RDI:
            mContext.Rdi = value;
            break;
        case R::DIL:
            set_uint8_lo(mContext.Rdi, value);
            break;
        case R::RBP:
            mContext.Rbp = value;
            break;
        case R::BPL:
            set_uint8_lo(mContext.Rbp, value);
            break;
        case R::RSP:
            mContext.Rsp = value;
            break;
        case R::SPL:
            set_uint8_lo(mContext.Rsp, value);
            break;
        case R::RIP:
            mContext.Rip = value;
            break;
        case R::R8:
            mContext.R8 = value;
            break;
        case R::R8D:
            set_uint32_lo(mContext.R8, value);
            break;
        case R::R8W:
            set_uint16_lo(mContext.R8, value);
            break;
        case R::R8B:
            set_uint8_lo(mContext.R8, value);
            break;
        case R::R9:
            mContext.R9 = value;
            break;
        case R::R9D:
            set_uint32_lo(mContext.R9, value);
            break;
        case R::R9W:
            set_uint16_lo(mContext.R9, value);
            break;
        case R::R9B:
            set_uint8_lo(mContext.R9, value);
            break;
        case R::R10:
            mContext.R10 = value;
            break;
        case R::R10D:
            set_uint32_lo(mContext.R10, value);
            break;
        case R::R10W:
            set_uint16_lo(mContext.R10, value);
            break;
        case R::R10B:
            set_uint8_lo(mContext.R10, value);
            break;
        case R::R11:
            mContext.R11 = value;
            break;
        case R::R11D:
            set_uint32_lo(mContext.R11, value);
            break;
        case R::R11W:
            set_uint16_lo(mContext.R11, value);
            break;
        case R::R11B:
            set_uint8_lo(mContext.R11, value);
            break;
        case R::R12:
            mContext.R12 = value;
            break;
        case R::R12D:
            set_uint32_lo(mContext.R12, value);
            break;
        case R::R12W:
            set_uint16_lo(mContext.R12, value);
            break;
        case R::R12B:
            set_uint8_lo(mContext.R12, value);
            break;
        case R::R13:
            mContext.R13 = value;
            break;
        case R::R13D:
            set_uint32_lo(mContext.R13, value);
            break;
        case R::R13W:
            set_uint16_lo(mContext.R13, value);
            break;
        case R::R13B:
            set_uint8_lo(mContext.R13, value);
            break;
        case R::R14:
            mContext.R14 = value;
            break;
        case R::R14D:
            set_uint32_lo(mContext.R14, value);
            break;
        case R::R14W:
            set_uint16_lo(mContext.R14, value);
            break;
        case R::R14B:
            set_uint8_lo(mContext.R14, value);
            break;
        case R::R15:
            mContext.R15 = value;
            break;
        case R::R15D:
            set_uint32_lo(mContext.R15, value);
            break;
        case R::R15W:
            set_uint16_lo(mContext.R15, value);
            break;
        case R::R15B:
            set_uint8_lo(mContext.R15, value);
            break;
#endif //_WIN64

        case R::GAX:
            contextGax = value;
            break;
        case R::GBX:
            contextGbx = value;
            break;
        case R::GCX:
            contextGcx = value;
            break;
        case R::GDX:
            contextGdx = value;
            break;
        case R::GDI:
            contextGdi = value;
            break;
        case R::GSI:
            contextGsi = value;
            break;
        case R::GBP:
            contextGbp = value;
            break;
        case R::GSP:
            contextGsp = value;
            break;
        case R::GIP:
            contextGip = value;
            break;

        case R::GS:
            mContext.SegGs = value;
            break;
        case R::FS:
            mContext.SegFs = value;
            break;
        case R::ES:
            mContext.SegEs = value;
            break;
        case R::DS:
            mContext.SegDs = value;
            break;
        case R::CS:
            mContext.SegCs = value;
            break;
        case R::SS:
            mContext.SegSs = value;
            break;

        default:
            __debugbreak();
        }
    }

    bool Registers::GetFlag(F flag)
    {
        handleLazyContext();

        return (mContext.EFlags & ptr(flag)) == ptr(flag);
    }

    void Registers::SetFlag(F flag, bool set)
    {
        handleLazyContext();

        if (set)
            mContext.EFlags |= ptr(flag);
        else
            mContext.EFlags &= ~ptr(flag);
    }

    void* Registers::getPtr(R reg)
    {
        handleLazyContext();

        switch (reg)
        {
        case R::DR0:
            return REGPTR(mContext.Dr0);
        case R::DR1:
            return REGPTR(mContext.Dr1);
        case R::DR2:
            return REGPTR(mContext.Dr2);
        case R::DR3:
            return REGPTR(mContext.Dr3);
        case R::DR6:
            return REGPTR(mContext.Dr6);
        case R::DR7:
            return REGPTR(mContext.Dr7);

        case R::EFlags:
            return REGPTR(mContext.EFlags);

        case R::EAX:
            return PTR_uint32_lo(contextGax);
        case R::AX:
            return PTR_uint16_lo(contextGax);
        case R::AH:
            return PTR_uint8_hi(contextGax);
        case R::AL:
            return PTR_uint8_lo(contextGax);
        case R::EBX:
            return PTR_uint32_lo(contextGbx);
        case R::BX:
            return PTR_uint16_lo(contextGbx);
        case R::BH:
            return PTR_uint8_hi(contextGbx);
        case R::BL:
            return PTR_uint8_lo(contextGbx);
        case R::ECX:
            return PTR_uint32_lo(contextGcx);
        case R::CX:
            return PTR_uint16_lo(contextGcx);
        case R::CH:
            return PTR_uint8_hi(contextGcx);
        case R::CL:
            return PTR_uint8_lo(contextGcx);
        case R::EDX:
            return PTR_uint32_lo(contextGdx);
        case R::DX:
            return PTR_uint16_lo(contextGdx);
        case R::DH:
            return PTR_uint8_hi(contextGdx);
        case R::DL:
            return PTR_uint8_lo(contextGdx);
        case R::EDI:
            return PTR_uint32_lo(contextGdi);
        case R::DI:
            return PTR_uint16_lo(contextGdi);
        case R::ESI:
            return PTR_uint32_lo(contextGsi);
        case R::SI:
            return PTR_uint16_lo(contextGsi);
        case R::EBP:
            return PTR_uint32_lo(contextGbp);
        case R::BP:
            return PTR_uint16_lo(contextGbp);
        case R::ESP:
            return PTR_uint32_lo(contextGsp);
        case R::SP:
            return PTR_uint16_lo(contextGsp);
        case R::EIP:
            return PTR_uint32_lo(contextGip);

#ifdef _WIN64
        case R::RAX:
            return REGPTR(mContext.Rax);
        case R::RBX:
            return REGPTR(mContext.Rbx);
        case R::RCX:
            return REGPTR(mContext.Rcx);
        case R::RDX:
            return REGPTR(mContext.Rdx);
        case R::RSI:
            return REGPTR(mContext.Rsi);
        case R::SIL:
            return PTR_uint8_lo(mContext.Rsi);
        case R::RDI:
            return REGPTR(mContext.Rdi);
        case R::DIL:
            return PTR_uint8_lo(mContext.Rdi);
        case R::RBP:
            return REGPTR(mContext.Rbp);
        case R::BPL:
            return PTR_uint8_lo(mContext.Rbp);
        case R::RSP:
            return REGPTR(mContext.Rsp);
        case R::SPL:
            return PTR_uint8_lo(mContext.Rsp);
        case R::RIP:
            return REGPTR(mContext.Rip);
        case R::R8:
            return REGPTR(mContext.R8);
        case R::R8D:
            return PTR_uint32_lo(mContext.R8);
        case R::R8W:
            return PTR_uint16_lo(mContext.R8);
        case R::R8B:
            return PTR_uint8_lo(mContext.R8);
        case R::R9:
            return REGPTR(mContext.R9);
        case R::R9D:
            return PTR_uint32_lo(mContext.R9);
        case R::R9W:
            return PTR_uint16_lo(mContext.R9);
        case R::R9B:
            return PTR_uint8_lo(mContext.R9);
        case R::R10:
            return REGPTR(mContext.R10);
        case R::R10D:
            return PTR_uint32_lo(mContext.R10);
        case R::R10W:
            return PTR_uint16_lo(mContext.R10);
        case R::R10B:
            return PTR_uint8_lo(mContext.R10);
        case R::R11:
            return REGPTR(mContext.R11);
        case R::R11D:
            return PTR_uint32_lo(mContext.R11);
        case R::R11W:
            return PTR_uint16_lo(mContext.R11);
        case R::R11B:
            return PTR_uint8_lo(mContext.R11);
        case R::R12:
            return REGPTR(mContext.R12);
        case R::R12D:
            return PTR_uint32_lo(mContext.R12);
        case R::R12W:
            return PTR_uint16_lo(mContext.R12);
        case R::R12B:
            return PTR_uint8_lo(mContext.R12);
        case R::R13:
            return REGPTR(mContext.R13);
        case R::R13D:
            return PTR_uint32_lo(mContext.R13);
        case R::R13W:
            return PTR_uint16_lo(mContext.R13);
        case R::R13B:
            return PTR_uint8_lo(mContext.R13);
        case R::R14:
            return REGPTR(mContext.R14);
        case R::R14D:
            return PTR_uint32_lo(mContext.R14);
        case R::R14W:
            return PTR_uint16_lo(mContext.R14);
        case R::R14B:
            return PTR_uint8_lo(mContext.R14);
        case R::R15:
            return REGPTR(mContext.R15);
        case R::R15D:
            return PTR_uint32_lo(mContext.R15);
        case R::R15W:
            return PTR_uint16_lo(mContext.R15);
        case R::R15B:
            return PTR_uint8_lo(mContext.R15);
#endif //_WIN64

        case R::GAX:
            return REGPTR(contextGax);
        case R::GBX:
            return REGPTR(contextGbx);
        case R::GCX:
            return REGPTR(contextGcx);
        case R::GDX:
            return REGPTR(contextGdx);
        case R::GDI:
            return REGPTR(contextGdi);
        case R::GSI:
            return REGPTR(contextGsi);
        case R::GBP:
            return REGPTR(contextGbp);
        case R::GSP:
            return REGPTR(contextGsp);
        case R::GIP:
            return REGPTR(contextGip);

        case R::GS:
            return REGPTR(mContext.SegGs);
        case R::FS:
            return REGPTR(mContext.SegFs);
        case R::ES:
            return REGPTR(mContext.SegEs);
        case R::DS:
            return REGPTR(mContext.SegDs);
        case R::CS:
            return REGPTR(mContext.SegCs);
        case R::SS:
            return REGPTR(mContext.SegSs);

        default:
            __debugbreak();
            return nullptr;
        }
    }
}