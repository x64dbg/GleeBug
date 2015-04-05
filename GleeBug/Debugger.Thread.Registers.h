#ifndef _DEBUGGER_THREAD_REGISTERS_H
#define _DEBUGGER_THREAD_REGISTERS_H

#include "Debugger.Global.h"

namespace GleeBug
{
	CONTEXT;
	/**
	\brief Thread register context.
	*/
	class RegistersInfo
	{
	public:
#ifdef _WIN64
		DWORD64 Rax;
		DWORD64 Rbx;
		DWORD64 Rcx;
		DWORD64 Rdx;
		DWORD64 Rsi;
		DWORD64 Rdi;
		DWORD64 Rbp;
		DWORD64 Rsp;
		DWORD64 Rip;
		DWORD64 R8;
		DWORD64 R9;
		DWORD64 R10;
		DWORD64 R11;
		DWORD64 R12;
		DWORD64 R13;
		DWORD64 R14;
		DWORD64 R15;
		DWORD EFlags;
#else //x86
		DWORD Eax;
		DWORD Ebx;
		DWORD Ecx;
		DWORD Edx;
		DWORD Esi;
		DWORD Edi;
		DWORD Ebp;
		DWORD Esp;
		DWORD Eip;
		DWORD EFlags;
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

		/**
		\brief Sets trap flag.
		\param set (Optional) true to set, false to unset.
		*/
		void SetTrapFlag(bool set = true);

		/**
		\brief Gets trap flag.
		\return true if the flag is set, false otherwise.
		*/
		bool GetTrapFlag();

		/**
		\brief Sets resume flag.
		\param set (Optional) true to set, false to unset.
		*/
		void SetResumeFlag(bool set = true);

		/**
		\brief Gets resume flag.
		\return true if the flag is set, false otherwise.
		*/
		bool GetResumeFlag();

	private:
		CONTEXT _context;
		const int TRAP_FLAG = 0x100;
		const int RESUME_FLAG = 0x10000;
	};
};

#endif //_DEBUGGER_THREAD_REGISTERS_H