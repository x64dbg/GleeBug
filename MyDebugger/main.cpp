#include <cstdio>
#include "MyDebugger.h"

/*
static ptr GetRegister(RegisterEnum reg)
{
    return 0;
}

static void SetRegister(RegisterEnum reg, ptr value)
{
}

template <RegisterEnum RegisterIndex, typename Type>
class Register
{
public:
    Type Get()
    {
        return reinterpret_cast<Type>(GetRegister(RegisterIndex));
    }

    void Set(Type value)
    {
        SetRegister(RegisterIndex, ptr(value));
    }
};

static void test()
{
    
}*/

int main()
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
    system("pause");
    return 0;
}