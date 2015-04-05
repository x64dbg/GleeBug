#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
	RegistersInfo::RegistersInfo()
	{
		memset(&this->_context, 0, sizeof(CONTEXT));
	}

	const CONTEXT* RegistersInfo::GetContext()
	{
#ifdef _WIN64
		_context.Rax = this->Rax;
		_context.Rbx = this->Rbx;
		_context.Rcx = this->Rcx;
		_context.Rdx = this->Rdx;
		_context.Rsi = this->Rsi;
		_context.Rdi = this->Rdi;
		_context.Rbp = this->Rbp;
		_context.Rsp = this->Rsp;
		_context.Rip = this->Rip;
		_context.R8 = this->R8;
		_context.R9 = this->R9;
		_context.R10 = this->R10;
		_context.R11 = this->R11;
		_context.R12 = this->R12;
		_context.R13 = this->R13;
		_context.R14 = this->R14;
		_context.R15 = this->R15;
		_context.EFlags = this->EFlags;
#else //x86
		_context.Eax = this->Eax;
		_context.Ebx = this->Ebx;
		_context.Ecx = this->Ecx;
		_context.Edx = this->Edx;
		_context.Esi = this->Esi;
		_context.Edi = this->Edi;
		_context.Ebp = this->Ebp;
		_context.Esp = this->Esp;
		_context.Eip = this->Eip;
		_context.EFlags = this->EFlags;
#endif //_WIN64
		return &_context;
	}

	void RegistersInfo::SetContext(const CONTEXT & context)
	{
#ifdef _WIN64
		this->Rax = context.Rax;
		this->Rbx = context.Rbx;
		this->Rcx = context.Rcx;
		this->Rdx = context.Rdx;
		this->Rsi = context.Rsi;
		this->Rdi = context.Rdi;
		this->Rbp = context.Rbp;
		this->Rsp = context.Rsp;
		this->Rip = context.Rip;
		this->R8 = context.R8;
		this->R9 = context.R9;
		this->R10 = context.R10;
		this->R11 = context.R11;
		this->R12 = context.R12;
		this->R13 = context.R13;
		this->R14 = context.R14;
		this->R15 = context.R15;
		this->EFlags = context.EFlags;
#else //x86
		this->Eax = context.Eax;
		this->Ebx = context.Ebx;
		this->Ecx = context.Ecx;
		this->Edx = context.Edx;
		this->Esi = context.Esi;
		this->Edi = context.Edi;
		this->Ebp = context.Ebp;
		this->Esp = context.Esp;
		this->Eip = context.Eip;
		this->EFlags = context.EFlags;
#endif //_WIN64
		this->_context = context;
	}

	void RegistersInfo::SetTrapFlag(bool set)
	{
		if (set)
			this->EFlags |= TRAP_FLAG;
		else
			this->EFlags &= ~TRAP_FLAG;
	}

	bool RegistersInfo::GetTrapFlag()
	{
		return (this->EFlags & TRAP_FLAG) == TRAP_FLAG;
	}

	void RegistersInfo::SetResumeFlag(bool set)
	{
		if (set)
			this->EFlags |= RESUME_FLAG;
		else
			this->EFlags &= ~RESUME_FLAG;
	}

	bool RegistersInfo::GetResumeFlag()
	{
		return (this->EFlags & RESUME_FLAG) == RESUME_FLAG;
	}
};