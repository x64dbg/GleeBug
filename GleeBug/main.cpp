#include <cstdio>
#include "Debugger.Core.h"
#include "Debugger.Loop.h"

int main()
{
	wchar_t szFilePath[256] = L"c:\\CodeBlocks\\arma_cert_bin_info\\bin\\arma_cert_bin_info.exe";
	wchar_t szCommandLine[256] = L"";
	wchar_t szCurrentDir[256] = L"c:\\CodeBlocks\\arma_cert_bin_info\\bin";
	Debugger::ProcessInfo process;
	if (Debugger::Init(szFilePath, NULL, szCurrentDir, &process))
	{
		printf("Debugger::Init success! PID: %X\n", process.ProcessId);
		Debugger::Loop();
	}
	else
	{
		puts("Debugger::Init failed!");
	}
	system("pause");
	return 0;
}