#ifndef _GLOBAL_ENGINE_CONTEXT_H
#define _GLOBAL_ENGINE_CONTEXT_H

#include "TitanEngine.h"

#undef CONTEXT_XSTATE

#if defined(_M_X64)
#define CONTEXT_XSTATE                      (0x00100040)
#else
#define CONTEXT_XSTATE                      (0x00010040)
#endif

#define XSTATE_AVX                          (XSTATE_GSSE)
#define XSTATE_MASK_AVX                     (XSTATE_MASK_GSSE)

typedef DWORD64(WINAPI* PGETENABLEDXSTATEFEATURES)();
typedef BOOL (WINAPI* PINITIALIZECONTEXT)(PVOID Buffer, DWORD ContextFlags, PCONTEXT* Context, PDWORD ContextLength);
typedef BOOL (WINAPI* PGETXSTATEFEATURESMASK)(PCONTEXT Context, PDWORD64 FeatureMask);
typedef PVOID(WINAPI* LOCATEXSTATEFEATURE)(PCONTEXT Context, DWORD FeatureId, PDWORD Length);
typedef BOOL (WINAPI* SETXSTATEFEATURESMASK)(PCONTEXT Context, DWORD64 FeatureMask);

extern PGETENABLEDXSTATEFEATURES _GetEnabledXStateFeatures;
extern PINITIALIZECONTEXT _InitializeContext;
extern PGETXSTATEFEATURESMASK _GetXStateFeaturesMask;
extern LOCATEXSTATEFEATURE _LocateXStateFeature;
extern SETXSTATEFEATURESMASK _SetXStateFeaturesMask;

bool _SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext, bool AVX_PRIORITY);
bool _GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext, bool avx);
bool InitXState(void);

#endif //_GLOBAL_ENGINE_CONTEXT_H