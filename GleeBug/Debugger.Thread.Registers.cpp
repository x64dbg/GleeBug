#include "Debugger.Thread.Registers.h"

namespace GleeBug
{
	RegistersInfo::RegistersInfo()
	{
		memset(&this->_context, 0, sizeof(CONTEXT));
	}

	const CONTEXT* RegistersInfo::GetContext()
	{
		return &_context;
	}

	void RegistersInfo::SetContext(const CONTEXT & context)
	{
		this->_context = context;
	}
};