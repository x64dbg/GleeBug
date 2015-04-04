#ifndef _DEBUGGER_THREAD_REGISTERS_H
#define _DEBUGGER_THREAD_REGISTERS_H

#include "Debugger.Global.h"

namespace GleeBug
{
	CONTEXT;
	/**
	\brief Thread register context. #lovethetrash
	*/
	struct RegistersInfo
	{
#ifdef _WIN64
		ULONG_PTR & Rax = _context.Rax;
		ULONG_PTR & Rbx = _context.Rbx;
		ULONG_PTR & Rcx = _context.Rcx;
		ULONG_PTR & Rdx = _context.Rdx;
		ULONG_PTR & Rsi = _context.Rsi;
		ULONG_PTR & Rdi = _context.Rdi;
		ULONG_PTR & Rbp = _context.Rbp;
		ULONG_PTR & Rsp = _context.Rsp;
		ULONG_PTR & Rip = _context.Rip;
		ULONG_PTR & R8 = _context.R8;
		ULONG_PTR & R9 = _context.R9;
		ULONG_PTR & R10 = _context.R10;
		ULONG_PTR & R11 = _context.R11;
		ULONG_PTR & R12 = _context.R12;
		ULONG_PTR & R13 = _context.R13;
		ULONG_PTR & R14 = _context.R14;
		ULONG_PTR & R15 = _context.R15;
#else //x86
		ULONG_PTR & Eax = _context.Eax;
		ULONG_PTR & Ebx = _context.Ebx;
		ULONG_PTR & Ecx = _context.Ecx;
		ULONG_PTR & Edx = _context.Edx;
		ULONG_PTR & Esi = _context.Esi;
		ULONG_PTR & Edi = _context.Edi;
		ULONG_PTR & Ebp = _context.Ebp;
		ULONG_PTR & Esp = _context.Esp;
		ULONG_PTR & Eip = _context.Eip;
#endif //_WIN64

		/**
		\brief Default constructor.
		*/
		RegistersInfo();

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
		CONTEXT _context;
	};
};

#endif //_DEBUGGER_THREAD_REGISTERS_H