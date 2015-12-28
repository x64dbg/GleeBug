#include <cstdio>
#include "MyDebugger.h"
#include "GleeBug/Static.File.h"
#include "GleeBug/Static.Pe.h"

#ifdef _WIN64
wchar_t szFilePath[256] = L"c:\\test64.exe";
#else //x86
wchar_t szFilePath[256] = L"c:\\test32.exe";
#endif //_WIN64

static void testDebugger()
{
    wchar_t szCommandLine[256] = L"";
    wchar_t szCurrentDir[256] = L"c:\\";
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
}

static void testStatic()
{
    using namespace GleeBug;
    File file(szFilePath, File::ReadOnly);
    if (file.Open())
    {
        Pe pe(file);
        if (pe.ParseHeaders())
        {
            PIMAGE_DOS_HEADER idh = pe.GetDosHeader().Data();
            puts("DOS Header:");
            printf("   e_magic: %02X\n", idh->e_magic);
            printf("  e_lfanew: %08X\n", idh->e_lfanew);
#ifdef _WIN64
            PIMAGE_NT_HEADERS64 inth = pe.GetNtHeaders64().Data();
#else //x32
            PIMAGE_NT_HEADERS32 inth = pe.GetNtHeaders32().Data();
#endif //_WIN64
            puts("\nNT Headers:");
            printf("  Signature: %08X\n", inth->Signature);
            PIMAGE_FILE_HEADER ifh = &inth->FileHeader;
            puts("\n  File Header:");
            printf("    Machine      : %02X\n", ifh->Machine);
            printf("    TimeDateStamp: %08X\n", ifh->TimeDateStamp);
            PIMAGE_OPTIONAL_HEADER ioh = &inth->OptionalHeader;
            puts("\n  Optional Header:");
            printf("    Magic    : %04X\n", ioh->Magic);
            printf("    ImageBase: %p\n", ioh->ImageBase);
            printf("    Subsystem: %04X\n", ioh->Subsystem);
            puts("\n  Section Headers:");
        }
        else
            puts("Pe::ParseHeaders failed!");
    }
    else
    {
        puts("File::Open failed!");
    }
}

int main()
{
    testStatic();
    system("pause");
    return 0;
}