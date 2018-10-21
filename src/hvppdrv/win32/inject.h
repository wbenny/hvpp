#pragma once
#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// Structures.
//////////////////////////////////////////////////////////////////////////

typedef struct _INJ_INJECTION_INFO
{
  LIST_ENTRY  ListEntry;

  //
  // Process ID.
  //
  HANDLE      ProcessId;

  //
  // Combination of INJ_SYSTEM_DLL flags indicating
  // which DLLs has been already loaded into this
  // process.
  //
  ULONG       LoadedDlls;

  //
  // If true, the process has been already injected.
  //
  BOOLEAN     IsInjected;

  //
  // If true, trigger of the queued user APC will be
  // immediately forced upon next kernel->user transition.
  //
  BOOLEAN     ForceUserApc;

  //
  // Address of LdrLoadDll routine within 32-bit ntdll.dll.
  //
  PVOID       LdrLoadDllX86;

#if defined(_M_AMD64)
  //
  // Address of LdrLoadDll routine within 64-bit ntdll.dll.
  //
  PVOID       LdrLoadDllX64;

  //
  // If true, 32-bit DLL will be injected into Wow64
  // processes.  If false, 64-bit DLL will be injected
  // into Wow64 processes.
  //
  BOOLEAN     UseWow64Injection;
#endif
} INJ_INJECTION_INFO, *PINJ_INJECTION_INFO;

//////////////////////////////////////////////////////////////////////////
// Public functions.
//////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
InjInitialize(
  _In_ PUNICODE_STRING DllPathX86,
  _In_ PUNICODE_STRING DllPathX64,
  _In_ BOOLEAN UseWow64Injection
  );

VOID
NTAPI
InjDestroy(
  VOID
  );

NTSTATUS
NTAPI
InjCreateInjectionInfo(
  _In_ HANDLE ProcessId
  );

VOID
NTAPI
InjRemoveInjectionInfo(
  _In_ HANDLE ProcessId
  );

PINJ_INJECTION_INFO
NTAPI
InjFindInjectionInfo(
  _In_ HANDLE ProcessId
  );

BOOLEAN
NTAPI
InjCanInject(
  _In_ PINJ_INJECTION_INFO InjectionInfo
  );

NTSTATUS
NTAPI
InjInject(
  _In_ PINJ_INJECTION_INFO InjectionInfo
  );

VOID
NTAPI
InjLoadImageNotifyRoutine(
  _In_opt_ PUNICODE_STRING FullImageName,
  _In_ HANDLE ProcessId,
  _In_ PIMAGE_INFO ImageInfo
  );

#ifdef __cplusplus
}
#endif
