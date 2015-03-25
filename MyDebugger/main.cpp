#include <cstdio>
#include "MyDebugger.h"

int main()
{
	wchar_t szFilePath[256] = L"c:\\CodeBlocks\\arma_cert_bin_info\\bin\\arma_cert_bin_info.exe";
	wchar_t szCommandLine[256] = L"";
	wchar_t szCurrentDir[256] = L"c:\\CodeBlocks\\arma_cert_bin_info\\bin";
	MyDebugger dbg;
	if (dbg.Init(szFilePath, szCommandLine, szCurrentDir))
	{
		puts("Debugger::Init success!");
		dbg.Start();
		puts("Debugger::Start finished!");
	}
	else
	{
		puts("Debugger::Init failed!");
	}
	system("pause");
	return 0;
}