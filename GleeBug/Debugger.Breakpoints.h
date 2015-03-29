#ifndef DEBUGGER_BREAKPOINTS_H
#define DEBUGGER_BREAKPOINTS_H

#include "Debugger.Global.h"


/*
Incomplete Job. I'll Continue Later. 
*/
namespace GleeBug{


	typedef std::tuple<uint32_t, LPVOID, uint32_t> breakpoint;
	typedef std::unordered_map<breakpoint, uint8_t> bpmap;

	struct BreakPointManager{

		bpmap breakpoints;

		BreakPointManager(){
			breakpoints = bpmap{};
		}

		bool AddBp(LPPROCESS_INFORMATION procinfo, LPVOID addr, uint32_t type){

			uint8_t bp_type;
			SIZE_T nbytes_written = 0;

			breakpoint bp( procinfo->dwProcessId, addr, type );

			switch (type)
			{
			case SOFT_BP:
				bp_type = 0xcc;
				break;
			default:
				return false;
			}

			if (ReadProcessMemory(procinfo->hProcess, addr, &bp_type, 1, &nbytes_written) == 0)
			{
				return false;
			}

			if (nbytes_written != 1){
				return false;
			}
			breakpoints[bp] = bp_type;


			if (WriteProcessMemory(procinfo->hProcess, addr, &bp_type, 1, &nbytes_written) == 0)
			{
				return false;
			}

			if (nbytes_written != 1){
				return false;
			}
			return true;
		}

		bool RemoveBp(LPPROCESS_INFORMATION proc_info, breakpoint bp){
			uint8_t original_instruction;
			SIZE_T nbytes_written = 0;
			try
			{
				original_instruction = breakpoints[bp];
			}
			catch (const std::out_of_range& oor){
				return false;
			}
			if (WriteProcessMemory(proc_info->hProcess, std::get<1>(bp), &std::get<2>(bp), 1, &nbytes_written) == 0)
			{
				return false;
			}
			if (nbytes_written != 1){
				return false;
			}
			return true;
		}

		bool DeleteBp(LPPROCESS_INFORMATION proc_info, breakpoint bp){
			bool success;

			success = RemoveBp(proc_info, bp);
			breakpoints.erase(bp);
			return success;
		}

		bool DisableAll()
		{

		}
	};

}
#endif