#include "Debugger.Loop.h"
#include "Debugger.Core.h"

namespace Debugger
{
	static void CreateProcessEvent(CREATE_PROCESS_DEBUG_INFO* CreateProcess, DebugState* state)
	{
		puts("> CreateProcessEvent");
	}

	static void ExitProcessEvent(EXIT_PROCESS_DEBUG_INFO* ExitProcess, DebugState* state)
	{
		puts("> ExitProcessEvent");
		if (state->DebugEvent.dwProcessId == state->Process.ProcessId)
		{
			state->BreakDebugger = true;
		}
	}

	static void CreateThreadEvent(CREATE_THREAD_DEBUG_INFO* CreateThread, DebugState* state)
	{
		puts("> CreateThreadEvent");
	}

	static void ExitThreadEvent(EXIT_THREAD_DEBUG_INFO* ExitThread, DebugState* state)
	{
		puts("> ExitThreadEvent");
	}

	static void LoadDllEvent(LOAD_DLL_DEBUG_INFO* LoadDll, DebugState* state)
	{
		puts("> LoadDllEvent");
	}

	static void UnloadDllEvent(UNLOAD_DLL_DEBUG_INFO* UnloadDll, DebugState* state)
	{
		puts("> UnloadDllEvent");
	}

	static void ExceptionEvent(EXCEPTION_DEBUG_INFO* Exception, DebugState* state)
	{
		puts("> ExceptionEvent");
	}

	static void DebugStringEvent(OUTPUT_DEBUG_STRING_INFO* DebugString, DebugState* state)
	{
		puts("> DebugStringEvent");
	}

	static void RipEvent(RIP_INFO* Rip, DebugState* state)
	{
		puts("> RipEvent");
	}

	void Loop()
	{
		DebugState* state = State();
		state->ContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
		while (!state->BreakDebugger)
		{
			if (!WaitForDebugEvent(&state->DebugEvent, INFINITE))
				break;

			switch (state->DebugEvent.dwDebugEventCode)
			{
			case CREATE_PROCESS_DEBUG_EVENT:
				CreateProcessEvent(&state->DebugEvent.u.CreateProcessInfo, state);
				break;
			case EXIT_PROCESS_DEBUG_EVENT:
				ExitProcessEvent(&state->DebugEvent.u.ExitProcess, state);
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				CreateThreadEvent(&state->DebugEvent.u.CreateThread, state);
				break;
			case EXIT_THREAD_DEBUG_EVENT:
				ExitThreadEvent(&state->DebugEvent.u.ExitThread, state);
				break;
			case LOAD_DLL_DEBUG_EVENT:
				LoadDllEvent(&state->DebugEvent.u.LoadDll, state);
				break;
			case UNLOAD_DLL_DEBUG_EVENT:
				UnloadDllEvent(&state->DebugEvent.u.UnloadDll, state);
				break;
			case EXCEPTION_DEBUG_EVENT:
				ExceptionEvent(&state->DebugEvent.u.Exception, state);
				break;
			case OUTPUT_DEBUG_STRING_EVENT:
				DebugStringEvent(&state->DebugEvent.u.DebugString, state);
				break;
			case RIP_EVENT:
				RipEvent(&state->DebugEvent.u.RipInfo, state);
				break;
			}

			if (!ContinueDebugEvent(state->DebugEvent.dwProcessId, state->DebugEvent.dwThreadId, state->ContinueStatus))
				break;
		}
	}
}