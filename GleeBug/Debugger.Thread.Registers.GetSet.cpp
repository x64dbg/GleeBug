#include "Debugger.Thread.Registers.h"

#ifdef _WIN64
#define contextGax _context.Rax
#define contextGbx _context.Rbx
#define contextGcx _context.Rcx
#define contextGdx _context.Rdx
#define contextGdi _context.Rdi
#define contextGsi _context.Rsi
#define contextGbp _context.Rbp
#define contextGsp _context.Rsp
#define contextGip _context.Rip
#else //x32
#define contextGax _context.Eax
#define contextGbx _context.Ebx
#define contextGcx _context.Ecx
#define contextGdx _context.Edx
#define contextGdi _context.Edi
#define contextGsi _context.Esi
#define contextGbp _context.Ebp
#define contextGsp _context.Esp
#define contextGip _context.Eip
#endif //_WIN64

#ifdef _WIN64
#define uint32_lo(x) ptr(x & 0xFFFFFFFF)
#else //x32
#define uint32_lo(x) ptr(x)
#endif //_WIN64
#define uint16_lo(x) ptr(x & 0xFFFFFFFF)
#define uint8_hi(x) ptr((x >> 8) & 0xFF)
#define uint8_lo(x) ptr(x & 0xFF)

#ifdef _WIN64
#define set_uint32_lo(x, y) x = (x & ~0xFFFFFFFF) | uint32_lo(y)
#else //x32
#define set_uint32_lo(x, y) x = y
#endif //_WIN64
#define set_uint16_lo(x, y) x = (x & ~0xFFFF) | uint16_lo(y)
#define set_uint8_hi(x, y) x = (x & ~0xFF00) | (uint8_lo(y) << 8)
#define set_uint8_lo(x, y) x = (x & ~0xFF) | uint8_lo(y)

namespace GleeBug
{
    ptr Registers::Get(R reg) const
    {
        switch (reg)
        {
        case R::DR0:
            return ptr(_context.Dr0);
        case R::DR1:
            return ptr(_context.Dr1);
        case R::DR2:
            return ptr(_context.Dr2);
        case R::DR3:
            return ptr(_context.Dr3);
        case R::DR6:
            return ptr(_context.Dr6);
        case R::DR7:
            return ptr(_context.Dr7);

        case R::EFlags:
            return ptr(_context.EFlags);

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
            return ptr(_context.Rax);
        case R::RBX:
            return ptr(_context.Rbx);
        case R::RCX:
            return ptr(_context.Rcx);
        case R::RDX:
            return ptr(_context.Rdx);
        case R::RSI:
            return ptr(_context.Rsi);
        case R::SIL:
            return uint8_lo(_context.Rsi);
        case R::RDI:
            return ptr(_context.Rdi);
        case R::DIL:
            return uint8_lo(_context.Rdi);
        case R::RBP:
            return ptr(_context.Rbp);
        case R::BPL:
            return uint8_lo(_context.Rbp);
        case R::RSP:
            return ptr(_context.Rsp);
        case R::SPL:
            return uint8_lo(_context.Rsp);
        case R::RIP:
            return ptr(_context.Rip);
        case R::R8:
            return ptr(_context.R8);
        case R::R8D:
            return uint32_lo(_context.R8);
        case R::R8W:
            return uint16_lo(_context.R8);
        case R::R8B:
            return uint8_lo(_context.R8);
        case R::R9:
            return ptr(_context.R9);
        case R::R9D:
            return uint32_lo(_context.R9);
        case R::R9W:
            return uint16_lo(_context.R9);
        case R::R9B:
            return uint8_lo(_context.R9);
        case R::R10:
            return ptr(_context.R10);
        case R::R10D:
            return uint32_lo(_context.R10);
        case R::R10W:
            return uint16_lo(_context.R10);
        case R::R10B:
            return uint8_lo(_context.R10);
        case R::R11:
            return ptr(_context.R11);
        case R::R11D:
            return uint32_lo(_context.R11);
        case R::R11W:
            return uint16_lo(_context.R11);
        case R::R11B:
            return uint8_lo(_context.R11);
        case R::R12:
            return ptr(_context.R12);
        case R::R12D:
            return uint32_lo(_context.R12);
        case R::R12W:
            return uint16_lo(_context.R12);
        case R::R12B:
            return uint8_lo(_context.R12);
        case R::R13:
            return ptr(_context.R13);
        case R::R13D:
            return uint32_lo(_context.R13);
        case R::R13W:
            return uint16_lo(_context.R13);
        case R::R13B:
            return uint8_lo(_context.R13);
        case R::R14:
            return ptr(_context.R14);
        case R::R14D:
            return uint32_lo(_context.R14);
        case R::R14W:
            return uint16_lo(_context.R14);
        case R::R14B:
            return uint8_lo(_context.R14);
        case R::R15:
            return ptr(_context.R15);
        case R::R15D:
            return uint32_lo(_context.R15);
        case R::R15W:
            return uint16_lo(_context.R15);
        case R::R15B:
            return uint8_lo(_context.R15);
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

        default:
            return 0;
        }
    }

    void Registers::Set(R reg, ptr value)
    {
        switch (reg)
        {
        case R::DR0:
            _context.Dr0 = value;
            break;
        case R::DR1:
            _context.Dr1 = value;
            break;
        case R::DR2:
            _context.Dr2 = value;
            break;
        case R::DR3:
            _context.Dr3 = value;
            break;
        case R::DR6:
            _context.Dr6 = value;
            break;
        case R::DR7:
            _context.Dr7 = value;
            break;

        case R::EFlags:
            _context.EFlags = (DWORD)value;
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
            _context.Rax = value;
            break;
        case R::RBX:
            _context.Rbx = value;
            break;
        case R::RCX:
            _context.Rcx = value;
            break;
        case R::RDX:
            _context.Rdx = value;
            break;
        case R::RSI:
            _context.Rsi = value;
            break;
        case R::SIL:
            set_uint8_lo(_context.Rsi, value);
            break;
        case R::RDI:
            _context.Rdi = value;
            break;
        case R::DIL:
            set_uint8_lo(_context.Rdi, value);
            break;
        case R::RBP:
            _context.Rbp = value;
            break;
        case R::BPL:
            set_uint8_lo(_context.Rbp, value);
            break;
        case R::RSP:
            _context.Rsp = value;
            break;
        case R::SPL:
            set_uint8_lo(_context.Rsp, value);
            break;
        case R::RIP:
            _context.Rip = value;
            break;
        case R::R8:
            _context.R8 = value;
            break;
        case R::R8D:
            set_uint32_lo(_context.R8, value);
            break;
        case R::R8W:
            set_uint16_lo(_context.R8, value);
            break;
        case R::R8B:
            set_uint8_lo(_context.R8, value);
            break;
        case R::R9:
            _context.R9 = value;
            break;
        case R::R9D:
            set_uint32_lo(_context.R9, value);
            break;
        case R::R9W:
            set_uint16_lo(_context.R9, value);
            break;
        case R::R9B:
            set_uint8_lo(_context.R9, value);
            break;
        case R::R10:
            _context.R10 = value;
            break;
        case R::R10D:
            set_uint32_lo(_context.R10, value);
            break;
        case R::R10W:
            set_uint16_lo(_context.R10, value);
            break;
        case R::R10B:
            set_uint8_lo(_context.R10, value);
            break;
        case R::R11:
            _context.R11 = value;
            break;
        case R::R11D:
            set_uint32_lo(_context.R11, value);
            break;
        case R::R11W:
            set_uint16_lo(_context.R11, value);
            break;
        case R::R11B:
            set_uint8_lo(_context.R11, value);
            break;
        case R::R12:
            _context.R12 = value;
            break;
        case R::R12D:
            set_uint32_lo(_context.R12, value);
            break;
        case R::R12W:
            set_uint16_lo(_context.R12, value);
            break;
        case R::R12B:
            set_uint8_lo(_context.R12, value);
            break;
        case R::R13:
            _context.R13 = value;
            break;
        case R::R13D:
            set_uint32_lo(_context.R13, value);
            break;
        case R::R13W:
            set_uint16_lo(_context.R13, value);
            break;
        case R::R13B:
            set_uint8_lo(_context.R13, value);
            break;
        case R::R14:
            _context.R14 = value;
            break;
        case R::R14D:
            set_uint32_lo(_context.R14, value);
            break;
        case R::R14W:
            set_uint16_lo(_context.R14, value);
            break;
        case R::R14B:
            set_uint8_lo(_context.R14, value);
            break;
        case R::R15:
            _context.R15 = value;
            break;
        case R::R15D:
            set_uint32_lo(_context.R15, value);
            break;
        case R::R15W:
            set_uint16_lo(_context.R15, value);
            break;
        case R::R15B:
            set_uint8_lo(_context.R15, value);
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
        }
    }
}