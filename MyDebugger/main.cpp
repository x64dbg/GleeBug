#include <cstdio>
#include "MyDebugger.h"
#include "GleeBug/Static.File.h"
#include "GleeBug/Static.Pe.h"
#include "GleeBug/Static.BufferFile.h"

static void testDebugger()
{
#ifdef _WIN64
    wchar_t szFilePath[256] = L"c:\\test64.exe";
#else //x86
    wchar_t szFilePath[256] = L"c:\\test32.exe";
#endif //_WIN64
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

template<typename T>
static void printRegion(const char* str, Region<T> region)
{
    printf("\n%s (offset: 0x%X, size: 0x%X, v: %s, e: %s)\n",
        str,
        region.Offset(),
        region.Size(),
        region.Valid() ? "true" : "false",
        region.Empty() ? "true" : "false");
}

static bool testPeFile(const wchar_t* szFileName, bool dumpData = true)
{
    using namespace GleeBug;
    auto result = false;
    File diskFile(szFileName, File::ReadOnly);
    if (diskFile.Open())
    {
        auto diskSize = diskFile.GetSize();
        std::vector<uint8> diskData(diskSize);
        if (diskFile.Read(0, diskData.data(), diskSize))
        {
            BufferFile file(diskData.data(), diskSize);
            Pe pe(file);
            auto parseError = pe.Parse(true);
            if (parseError == Pe::ErrorOk)
            {
                result = true;
                if (!dumpData)
                    return result;
                auto idh = pe.GetDosHeader();
                printRegion("DOS Header:", idh);
                printf("   e_magic: %02X\n", idh->e_magic);
                printf("  e_lfanew: %08X\n", idh->e_lfanew);

                auto afterDosData = pe.GetAfterDosData();
                printRegion("After DOS Data", afterDosData);

#ifdef _WIN64
                auto inth = pe.GetNtHeaders64();
#else //x32
                auto inth = pe.GetNtHeaders32();
#endif //_WIN64
                printRegion("NT Headers:", inth);
                printf("  Signature: %08X\n", inth->Signature);

                PIMAGE_FILE_HEADER ifh = &inth->FileHeader;
                puts("\n  File Header:");
                printf("    Machine             : %04X\n", ifh->Machine);
                printf("    NumberOfSections    : %04X\n", ifh->NumberOfSections);
                printf("    TimeDateStamp       : %08X\n", ifh->TimeDateStamp);
                printf("    SizeOfOptionalHeader: %04X\n", ifh->SizeOfOptionalHeader);

                PIMAGE_OPTIONAL_HEADER ioh = &inth->OptionalHeader;
                puts("\n  Optional Header:");
                printf("    Magic     : %04X\n", ioh->Magic);
                printf("    EntryPoint: %08X\n", ioh->AddressOfEntryPoint);
                printf("    ImageBase : %p\n", ioh->ImageBase);
                printf("    Subsystem : %04X\n", ioh->Subsystem);

                auto afterOptionalData = pe.GetAfterOptionalData();
                printRegion("After Optional Data", afterOptionalData);

                auto ish = pe.GetSectionHeaders();
                printRegion("Section Headers:", ish);
                for (auto i = 0; i < ifh->NumberOfSections; i++)
                {
                    if (i)
                        puts("");
                    auto cur = ish.Data() + i;
                    printf("  Section %d:\n", i + 1);
                    char name[9] = "";
                    memcpy(name, cur->Name, sizeof(cur->Name));
                    printf("    Name : %s\n", name);
                    printf("    VSize: %08X\n", cur->Misc.VirtualSize);
                    printf("    VAddr: %08X\n", cur->VirtualAddress);
                    printf("    RSize: %08X\n", cur->SizeOfRawData);
                    printf("    RAddr: %08X\n", cur->PointerToRawData);
                }

                auto afterSectionHeadersData = pe.GetAfterSectionHeadersData();
                printRegion("After Section Headers Data", afterSectionHeadersData);
            }
            else
                printf("Pe::Parse failed (%s)!\n", pe.ErrorText(parseError));
        }
        else
            puts("File::Read failed!");
    }
    else
        puts("File::Open failed!");
    return result;
}

static void testCorkami()
{
#include "PeTests.h"
    wchar_t szBasePath[MAX_PATH] = L"c:\\!exclude\\pe\\bin\\";
    int okCount = 0;
    for (auto i = 0; i < _countof(peTestFiles); i++)
    {
        std::wstring fileName(szBasePath);
        fileName += peTestFiles[i];
        if (testPeFile(fileName.c_str(), false))
            okCount++;
        else
        {
            printf("file: %ws\n\n", fileName.c_str());
        }
    }
    printf("\n%d/%d parsed OK!\n", okCount, _countof(peTestFiles));
}

int main()
{
    testPeFile(L"C:\\test64.exe");
    //testCorkami();
    puts("");
    system("pause");
    return 0;
}