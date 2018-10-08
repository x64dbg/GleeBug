#include "Global.Engine.Context.h"
#include <vector>

typedef int(*p_printf)(const char*, ...);

static p_printf hax()
{
    auto d = p_printf(GetProcAddress(GetModuleHandleW(L"x64dbg.dll"), "_plugin_logprintf"));
    return d ? d : printf;
}

static auto dprintf = hax();

#ifdef _WIN64
//https://stackoverflow.com/a/869597/1806760
template<typename T> struct identity
{
    typedef T type;
};

template<typename Dst> Dst implicit_cast(typename identity<Dst>::type t)
{
    return t;
}

//https://github.com/electron/crashpad/blob/4054e6cba3ba023d9c00260518ec2912607ae17c/snapshot/cpu_context.cc
enum
{
    kX87TagValid = 0,
    kX87TagZero,
    kX87TagSpecial,
    kX87TagEmpty,
};

typedef uint8_t X87Register[10];

union X87OrMMXRegister
{
    struct
    {
        X87Register st;
        uint8_t st_reserved[6];
    };
    struct
    {
        uint8_t mm_value[8];
        uint8_t mm_reserved[8];
    };
};

static_assert(sizeof(X87OrMMXRegister) == sizeof(M128A), "sizeof(X87OrMMXRegister) != sizeof(M128A)");

static uint16_t FxsaveToFsaveTagWord(
    uint16_t fsw,
    uint8_t fxsave_tag,
    const X87OrMMXRegister* st_mm)
{
    // The x87 tag word (in both abridged and full form) identifies physical
    // registers, but |st_mm| is arranged in logical stack order. In order to map
    // physical tag word bits to the logical stack registers they correspond to,
    // the "stack top" value from the x87 status word is necessary.
    int stack_top = (fsw >> 11) & 0x7;

    uint16_t fsave_tag = 0;
    for(int physical_index = 0; physical_index < 8; ++physical_index)
    {
        bool fxsave_bit = (fxsave_tag & (1 << physical_index)) != 0;
        uint8_t fsave_bits;

        if(fxsave_bit)
        {
            int st_index = (physical_index + 8 - stack_top) % 8;
            const X87Register & st = st_mm[st_index].st;

            uint32_t exponent = ((st[9] & 0x7f) << 8) | st[8];
            if(exponent == 0x7fff)
            {
                // Infinity, NaN, pseudo-infinity, or pseudo-NaN. If it was important to
                // distinguish between these, the J bit and the M bit (the most
                // significant bit of |fraction|) could be consulted.
                fsave_bits = kX87TagSpecial;
            }
            else
            {
                // The integer bit the "J bit".
                bool integer_bit = (st[7] & 0x80) != 0;
                if(exponent == 0)
                {
                    uint64_t fraction = ((implicit_cast<uint64_t>(st[7]) & 0x7f) << 56) |
                                        (implicit_cast<uint64_t>(st[6]) << 48) |
                                        (implicit_cast<uint64_t>(st[5]) << 40) |
                                        (implicit_cast<uint64_t>(st[4]) << 32) |
                                        (implicit_cast<uint32_t>(st[3]) << 24) |
                                        (st[2] << 16) | (st[1] << 8) | st[0];
                    if(!integer_bit && fraction == 0)
                    {
                        fsave_bits = kX87TagZero;
                    }
                    else
                    {
                        // Denormal (if the J bit is clear) or pseudo-denormal.
                        fsave_bits = kX87TagSpecial;
                    }
                }
                else if(integer_bit)
                {
                    fsave_bits = kX87TagValid;
                }
                else
                {
                    // Unnormal.
                    fsave_bits = kX87TagSpecial;
                }
            }
        }
        else
        {
            fsave_bits = kX87TagEmpty;
        }

        fsave_tag |= (fsave_bits << (physical_index * 2));
    }

    return fsave_tag;
}

static uint8_t FsaveToFxsaveTagWord(uint16_t fsave_tag)
{
    uint8_t fxsave_tag = 0;
    for(int physical_index = 0; physical_index < 8; ++physical_index)
    {
        const uint8_t fsave_bits = (fsave_tag >> (physical_index * 2)) & 0x3;
        const bool fxsave_bit = fsave_bits != kX87TagEmpty;
        fxsave_tag |= fxsave_bit << physical_index;
    }
    return fxsave_tag;
}
#endif //_WIN64

PGETENABLEDXSTATEFEATURES _GetEnabledXStateFeatures = NULL;
PINITIALIZECONTEXT _InitializeContext = NULL;
PGETXSTATEFEATURESMASK _GetXStateFeaturesMask = NULL;
LOCATEXSTATEFEATURE _LocateXStateFeature = NULL;
SETXSTATEFEATURESMASK _SetXStateFeaturesMask = NULL;

static bool SetAVXContext(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    if (InitXState() == false)
        return false;

    DWORD64 FeatureMask = _GetEnabledXStateFeatures();
    if ((FeatureMask & XSTATE_MASK_AVX) == 0)
        return false;

    DWORD ContextSize = 0;
    BOOL Success = _InitializeContext(NULL,
        CONTEXT_ALL | CONTEXT_XSTATE,
        NULL,
        &ContextSize);

    if ((Success == TRUE) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return false;

    std::vector<unsigned char> dataBuffer;
    dataBuffer.resize(ContextSize);
    PVOID Buffer = dataBuffer.data();
    if (Buffer == NULL)
        return false;

    PCONTEXT Context;
    Success = _InitializeContext(Buffer,
        CONTEXT_ALL | CONTEXT_XSTATE,
        &Context,
        &ContextSize);
    if (Success == FALSE)
        return false;

    if (_SetXStateFeaturesMask(Context, XSTATE_MASK_AVX) == FALSE)
        return false;

    if (GetThreadContext(hActiveThread, Context) == FALSE)
        return false;

    if (_GetXStateFeaturesMask(Context, &FeatureMask) == FALSE)
        return false;

    DWORD FeatureLength;
    XmmRegister_t* Sse = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_LEGACY_SSE, &FeatureLength);
    XmmRegister_t* Avx = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX, NULL);
    int NumberOfRegisters = FeatureLength / sizeof(Sse[0]);

    if (Sse != NULL) //If the feature is unsupported by the processor it will return NULL
    {
        for (int i = 0; i < NumberOfRegisters; i++)
            Sse[i] = titcontext->YmmRegisters[i].Low;
    }

    if (Avx != NULL) //If the feature is unsupported by the processor it will return NULL
    {
        for (int i = 0; i < NumberOfRegisters; i++)
            Avx[i] = titcontext->YmmRegisters[i].High;
    }

    return (SetThreadContext(hActiveThread, Context) == TRUE);
}

static bool GetAVXContext(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext)
{
    if (InitXState() == false)
        return false;

    DWORD64 FeatureMask = _GetEnabledXStateFeatures();
    if ((FeatureMask & XSTATE_MASK_AVX) == 0)
        return false;

    DWORD ContextSize = 0;
    BOOL Success = _InitializeContext(NULL,
        CONTEXT_ALL | CONTEXT_XSTATE,
        NULL,
        &ContextSize);

    if ((Success == TRUE) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return false;

    std::vector<unsigned char> dataBuffer;
    dataBuffer.resize(ContextSize);
    PVOID Buffer = dataBuffer.data();
    if (Buffer == NULL)
        return false;

    PCONTEXT Context;
    Success = _InitializeContext(Buffer,
        CONTEXT_ALL | CONTEXT_XSTATE,
        &Context,
        &ContextSize);
    if (Success == FALSE)
        return false;

    if (_SetXStateFeaturesMask(Context, XSTATE_MASK_AVX) == FALSE)
        return false;

    if (GetThreadContext(hActiveThread, Context) == FALSE)
        return false;

    if (_GetXStateFeaturesMask(Context, &FeatureMask) == FALSE)
        return false;

    DWORD FeatureLength;
    XmmRegister_t* Sse = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_LEGACY_SSE, &FeatureLength);
    XmmRegister_t* Avx = (XmmRegister_t*)_LocateXStateFeature(Context, XSTATE_AVX, NULL);
    int NumberOfRegisters = FeatureLength / sizeof(Sse[0]);

    if (Sse != NULL) //If the feature is unsupported by the processor it will return NULL
    {
        for (int i = 0; i < NumberOfRegisters; i++)
            titcontext->YmmRegisters[i].Low = Sse[i];
    }

    if (Avx != NULL) //If the feature is unsupported by the processor it will return NULL
    {
        for (int i = 0; i < NumberOfRegisters; i++)
            titcontext->YmmRegisters[i].High = Avx[i];
    }

    return true;
}

bool _SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext, bool AVX_PRIORITY)
{
    CONTEXT DBGContext;
    memset(&DBGContext, 0, sizeof(DBGContext));

    DBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

    if (!GetThreadContext(hActiveThread, &DBGContext))
        return false;

    DBGContext.EFlags = (DWORD)titcontext->eflags;
    DBGContext.Dr0 = titcontext->dr0;
    DBGContext.Dr1 = titcontext->dr1;
    DBGContext.Dr2 = titcontext->dr2;
    DBGContext.Dr3 = titcontext->dr3;
    DBGContext.Dr6 = titcontext->dr6;
    DBGContext.Dr7 = titcontext->dr7;
    DBGContext.SegGs = titcontext->gs;
    DBGContext.SegFs = titcontext->fs;
    DBGContext.SegEs = titcontext->es;
    DBGContext.SegDs = titcontext->ds;
    DBGContext.SegCs = titcontext->cs;
    DBGContext.SegSs = titcontext->ss;

#ifdef _WIN64 //x64
    DBGContext.Rax = titcontext->cax;
    DBGContext.Rbx = titcontext->cbx;
    DBGContext.Rcx = titcontext->ccx;
    DBGContext.Rdx = titcontext->cdx;
    DBGContext.Rdi = titcontext->cdi;
    DBGContext.Rsi = titcontext->csi;
    DBGContext.Rbp = titcontext->cbp;
    DBGContext.Rsp = titcontext->csp;
    DBGContext.Rip = titcontext->cip;
    DBGContext.R8 = titcontext->r8;
    DBGContext.R9 = titcontext->r9;
    DBGContext.R10 = titcontext->r10;
    DBGContext.R11 = titcontext->r11;
    DBGContext.R12 = titcontext->r12;
    DBGContext.R13 = titcontext->r13;
    DBGContext.R14 = titcontext->r14;
    DBGContext.R15 = titcontext->r15;

    DBGContext.FltSave.ControlWord = titcontext->x87fpu.ControlWord;
    DBGContext.FltSave.StatusWord = titcontext->x87fpu.StatusWord;
    DBGContext.FltSave.TagWord = FsaveToFxsaveTagWord(titcontext->x87fpu.TagWord);
    DBGContext.FltSave.ErrorSelector = (WORD)titcontext->x87fpu.ErrorSelector;
    DBGContext.FltSave.ErrorOffset = titcontext->x87fpu.ErrorOffset;
    DBGContext.FltSave.DataSelector = (WORD)titcontext->x87fpu.DataSelector;
    DBGContext.FltSave.DataOffset = titcontext->x87fpu.DataOffset;
    // Skip titcontext->x87fpu.Cr0NpxState
    DBGContext.MxCsr = titcontext->MxCsr;

    for(int i = 0; i < 8; i++)
        memcpy(& DBGContext.FltSave.FloatRegisters[i], &(titcontext->RegisterArea[i * 10]), 10);

    for(int i = 0; i < 16; i++)
        memcpy(& (DBGContext.FltSave.XmmRegisters[i]), & (titcontext->XmmRegisters[i]), 16);

#else //x86
    DBGContext.Eax = titcontext->cax;
    DBGContext.Ebx = titcontext->cbx;
    DBGContext.Ecx = titcontext->ccx;
    DBGContext.Edx = titcontext->cdx;
    DBGContext.Edi = titcontext->cdi;
    DBGContext.Esi = titcontext->csi;
    DBGContext.Ebp = titcontext->cbp;
    DBGContext.Esp = titcontext->csp;
    DBGContext.Eip = titcontext->cip;

    DBGContext.FloatSave.ControlWord = titcontext->x87fpu.ControlWord;
    DBGContext.FloatSave.StatusWord = titcontext->x87fpu.StatusWord;
    DBGContext.FloatSave.TagWord = titcontext->x87fpu.TagWord;
    DBGContext.FloatSave.ErrorSelector = titcontext->x87fpu.ErrorSelector;
    DBGContext.FloatSave.ErrorOffset = titcontext->x87fpu.ErrorOffset;
    DBGContext.FloatSave.DataSelector = titcontext->x87fpu.DataSelector;
    DBGContext.FloatSave.DataOffset = titcontext->x87fpu.DataOffset;
    DBGContext.FloatSave.Cr0NpxState = titcontext->x87fpu.Cr0NpxState;

    memcpy(DBGContext.FloatSave.RegisterArea, titcontext->RegisterArea, 80);

    // MXCSR ExtendedRegisters[24]
    memcpy(& (DBGContext.ExtendedRegisters[24]), & titcontext->MxCsr, sizeof(titcontext->MxCsr));

    // for x86 copy the 8 Xmm Registers from ExtendedRegisters[(10+n)*16]; (n is the index of the xmm register) to the XMM register
    for(int i = 0; i < 8; i++)
        memcpy(& DBGContext.ExtendedRegisters[(10 + i) * 16], &(titcontext->XmmRegisters[i]), 16);
#endif

    bool returnf = !!SetThreadContext(hActiveThread, &DBGContext);

    if (AVX_PRIORITY)
        SetAVXContext(hActiveThread, titcontext);

    return returnf;
}

bool _GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext, bool avx)
{
    CONTEXT DBGContext;
    memset(&DBGContext, 0, sizeof(CONTEXT));
    memset(titcontext, 0, sizeof(TITAN_ENGINE_CONTEXT_t));

    DBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

    if(!GetThreadContext(hActiveThread, &DBGContext))
        return false;

    titcontext->eflags = DBGContext.EFlags;
    titcontext->dr0 = DBGContext.Dr0;
    titcontext->dr1 = DBGContext.Dr1;
    titcontext->dr2 = DBGContext.Dr2;
    titcontext->dr3 = DBGContext.Dr3;
    titcontext->dr6 = DBGContext.Dr6;
    titcontext->dr7 = DBGContext.Dr7;
    titcontext->gs = (unsigned short) DBGContext.SegGs;
    titcontext->fs = (unsigned short) DBGContext.SegFs;
    titcontext->es = (unsigned short) DBGContext.SegEs;
    titcontext->ds = (unsigned short) DBGContext.SegDs;
    titcontext->cs = (unsigned short) DBGContext.SegCs;
    titcontext->ss = (unsigned short) DBGContext.SegSs;

#ifdef _WIN64 //x64
    titcontext->cax = DBGContext.Rax;
    titcontext->cbx = DBGContext.Rbx;
    titcontext->ccx = DBGContext.Rcx;
    titcontext->cdx = DBGContext.Rdx;
    titcontext->cdi = DBGContext.Rdi;
    titcontext->csi = DBGContext.Rsi;
    titcontext->cbp = DBGContext.Rbp;
    titcontext->csp = DBGContext.Rsp;
    titcontext->cip = DBGContext.Rip;
    titcontext->r8 = DBGContext.R8;
    titcontext->r9 = DBGContext.R9;
    titcontext->r10 = DBGContext.R10;
    titcontext->r11 = DBGContext.R11;
    titcontext->r12 = DBGContext.R12;
    titcontext->r13 = DBGContext.R13;
    titcontext->r14 = DBGContext.R14;
    titcontext->r15 = DBGContext.R15;

    titcontext->x87fpu.ControlWord = DBGContext.FltSave.ControlWord;
    titcontext->x87fpu.StatusWord = DBGContext.FltSave.StatusWord;
    titcontext->x87fpu.TagWord = FxsaveToFsaveTagWord(DBGContext.FltSave.StatusWord, DBGContext.FltSave.TagWord, (const X87OrMMXRegister*)DBGContext.FltSave.FloatRegisters);
    titcontext->x87fpu.ErrorSelector = DBGContext.FltSave.ErrorSelector;
    titcontext->x87fpu.ErrorOffset = DBGContext.FltSave.ErrorOffset;
    titcontext->x87fpu.DataSelector = DBGContext.FltSave.DataSelector;
    titcontext->x87fpu.DataOffset = DBGContext.FltSave.DataOffset;
    // Skip titcontext->x87fpu.Cr0NpxState (https://github.com/x64dbg/x64dbg/issues/255)
    titcontext->MxCsr = DBGContext.MxCsr;

    for(int i = 0; i < 8; i++)
        memcpy(&titcontext->RegisterArea[i * 10], &DBGContext.FltSave.FloatRegisters[i], 10);

    for(int i = 0; i < 16; i++)
        memcpy(&titcontext->XmmRegisters[i], &DBGContext.FltSave.XmmRegisters[i], 16);

#else //x86
    titcontext->cax = DBGContext.Eax;
    titcontext->cbx = DBGContext.Ebx;
    titcontext->ccx = DBGContext.Ecx;
    titcontext->cdx = DBGContext.Edx;
    titcontext->cdi = DBGContext.Edi;
    titcontext->csi = DBGContext.Esi;
    titcontext->cbp = DBGContext.Ebp;
    titcontext->csp = DBGContext.Esp;
    titcontext->cip = DBGContext.Eip;

    titcontext->x87fpu.ControlWord = (WORD) DBGContext.FloatSave.ControlWord;
    titcontext->x87fpu.StatusWord = (WORD) DBGContext.FloatSave.StatusWord;
    titcontext->x87fpu.TagWord = (WORD) DBGContext.FloatSave.TagWord;
    titcontext->x87fpu.ErrorSelector = DBGContext.FloatSave.ErrorSelector;
    titcontext->x87fpu.ErrorOffset = DBGContext.FloatSave.ErrorOffset;
    titcontext->x87fpu.DataSelector = DBGContext.FloatSave.DataSelector;
    titcontext->x87fpu.DataOffset = DBGContext.FloatSave.DataOffset;
    titcontext->x87fpu.Cr0NpxState = DBGContext.FloatSave.Cr0NpxState;

    memcpy(titcontext->RegisterArea, DBGContext.FloatSave.RegisterArea, 80);

    // MXCSR ExtendedRegisters[24]
    memcpy(& (titcontext->MxCsr), & (DBGContext.ExtendedRegisters[24]), sizeof(titcontext->MxCsr));

    // for x86 copy the 8 Xmm Registers from ExtendedRegisters[(10+n)*16]; (n is the index of the xmm register) to the XMM register
    for(int i = 0; i < 8; i++)
        memcpy(&(titcontext->XmmRegisters[i]),  & DBGContext.ExtendedRegisters[(10 + i) * 16], 16);
#endif

    if(avx)
        GetAVXContext(hActiveThread, titcontext);

    return true;
}

bool InitXState()
{
    static bool init = false;
    if(!init)
    {
        init = true;
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if(kernel32 != NULL)
        {
            _GetEnabledXStateFeatures = (PGETENABLEDXSTATEFEATURES)GetProcAddress(kernel32, "GetEnabledXStateFeatures");
            _InitializeContext = (PINITIALIZECONTEXT)GetProcAddress(kernel32, "InitializeContext");
            _GetXStateFeaturesMask = (PGETXSTATEFEATURESMASK)GetProcAddress(kernel32, "GetXStateFeaturesMask");
            _LocateXStateFeature = (LOCATEXSTATEFEATURE)GetProcAddress(kernel32, "LocateXStateFeature");
            _SetXStateFeaturesMask = (SETXSTATEFEATURESMASK)GetProcAddress(kernel32, "SetXStateFeaturesMask");
        }
    }
    return (_GetEnabledXStateFeatures != NULL &&
            _InitializeContext != NULL &&
            _GetXStateFeaturesMask != NULL &&
            _LocateXStateFeature != NULL &&
            _SetXStateFeaturesMask != NULL);
}