#include "Debugger.h"

namespace GleeBug
{
	void Debugger::log(std::string msg)
	{
		puts(msg.c_str());
	}
};