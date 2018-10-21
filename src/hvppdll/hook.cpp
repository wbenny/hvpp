#include "hook.h"
#include "device.h"

// #undef C_ASSERT
// #define C_ASSERT_JOIN2(a, b) a ## b
// #define C_ASSERT_JOIN(a, b) C_ASSERT_JOIN2(a, b)
// #define C_ASSERT(e) struct C_ASSERT_JOIN(__DUMMY__, __LINE__) { char C_ASSERT_JOIN(__DUMMY__, __LINE__)[(e)?1:-1]; }
#include <detours.h>

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTE_OFFSET(Va) ((ULONG)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))

#pragma section(".bkp", read, write)
#pragma section(".hook", read, write)

#pragma section(".hook$A", read, write)
#pragma section(".hook$M", read, write)
#pragma section(".hook$Z", read, write)

typedef struct _HOOK_METADATA
{
  PVOID* OriginalFunction;
  PVOID HookFunction;
  PVOID BackupPage;
  PVOID RealTarget;
  ULONG Offset;
} HOOK_METADATA, *PHOOK_METADATA;

__declspec(allocate(".hook$A")) PHOOK_METADATA MetaBegin = NULL;
__declspec(allocate(".hook$Z")) PHOOK_METADATA MetaEnd   = NULL;

#define HOOK_DEFINE(FunctionName)                                       \
  decltype(FunctionName)* Orig ## FunctionName = &FunctionName;         \
  decltype(FunctionName)  Hook ## FunctionName;                         \
                                                                        \
  __declspec(allocate(".bkp"))                                          \
  __declspec(align(PAGE_SIZE))                                          \
  UCHAR Bkp ## FunctionName[PAGE_SIZE];                                 \
                                                                        \
  HOOK_METADATA InternalMeta ## FunctionName = {                        \
    (PVOID*)&Orig ## FunctionName,                                      \
    (PVOID)&Hook ## FunctionName,                                       \
    (PVOID)Bkp ## FunctionName,                                         \
    DetourCodeFromPointer((PVOID)&FunctionName, NULL),                  \
    BYTE_OFFSET(&FunctionName)                                          \
  };                                                                    \
                                                                        \
  __declspec(allocate(".hook$M"))                                       \
  PHOOK_METADATA Meta ## FunctionName = &InternalMeta ## FunctionName;

#include "hook.inl"

NTSTATUS
NTAPI
HookInitialize(
  VOID
  )
{
  DetourTransactionBegin();

  for (PHOOK_METADATA* Iterator = &MetaBegin; Iterator != &MetaEnd; ++Iterator)
  {
    if (*Iterator != NULL)
    {
      PHOOK_METADATA Metadata = *Iterator;

      RtlCopyMemory(Metadata->BackupPage, PAGE_ALIGN(Metadata->RealTarget), PAGE_SIZE);
      DetourAttach(Metadata->OriginalFunction, Metadata->HookFunction);
    }
  }

  DetourTransactionCommit();

  for (PHOOK_METADATA* Iterator = &MetaBegin; Iterator != &MetaEnd; ++Iterator)
  {
    if (*Iterator != NULL)
    {
      PHOOK_METADATA Metadata = *Iterator;

      DeviceShadowPageAdd(Metadata->BackupPage, Metadata->RealTarget, Metadata->Offset);
    }
  }

  DeviceShadowPageApply();

  return STATUS_SUCCESS;
}

VOID
NTAPI
HookDestroy(
  VOID
  )
{
  DetourTransactionBegin();

  for (PHOOK_METADATA* Iterator = &MetaBegin; Iterator != &MetaEnd; ++Iterator)
  {
    if (*Iterator != NULL)
    {
      PHOOK_METADATA Metadata = *Iterator;

      DetourDetach(Metadata->OriginalFunction, Metadata->HookFunction);
    }
  }

  DetourTransactionCommit();
}
