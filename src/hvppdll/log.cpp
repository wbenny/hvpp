#include "log.h"

//
// Unfortunatelly sprintf-like functions are not exposed
// by ntdll.lib, which we're linking against.  We have to
// load them dynamically.
//

using _snwprintf_fn_t = int(__cdecl*)(
  wchar_t *buffer,
  size_t count,
  const wchar_t *format,
  ...
  );

inline _snwprintf_fn_t ntdll_snwprintf = nullptr;

using _vsnwprintf_fn_t = int(__cdecl*)(
  wchar_t *buffer,
  size_t count,
  const wchar_t *format,
  va_list argptr
  );

inline _vsnwprintf_fn_t ntdll_vsnwprintf = nullptr;

//
// ETW provider GUID and global provider handle.
//

//
// GUID:
//   {a4b4ba50-a667-43f5-919b-1e52a6d69bd5}
//

GUID ProviderGuid = {
  0xa4b4ba50, 0xa667, 0x43f5, { 0x91, 0x9b, 0x1e, 0x52, 0xa6, 0xd6, 0x9b, 0xd5 }
};

REGHANDLE ProviderHandle;

NTSTATUS
NTAPI
LogInitialize(
  VOID
  )
{
  //
  // First, resolve address of the _(v)snwprintf functions.
  //

  UNICODE_STRING NtdllPath;
  RtlInitUnicodeString(&NtdllPath, (PWSTR)L"ntdll.dll");

  //
  // Until something really tragic happens, following functions
  // should never fail.
  //

  HANDLE NtdllHandle;
  LdrGetDllHandle(NULL, 0, &NtdllPath, &NtdllHandle);

  ANSI_STRING RoutineName;

  RtlInitAnsiString(&RoutineName, (PSTR)"_snwprintf");
  LdrGetProcedureAddress(NtdllHandle, &RoutineName, 0, (PVOID*)&ntdll_snwprintf);

  RtlInitAnsiString(&RoutineName, (PSTR)"_vsnwprintf");
  LdrGetProcedureAddress(NtdllHandle, &RoutineName, 0, (PVOID*)&ntdll_vsnwprintf);

  //
  // Register ETW provider.
  //

  return EtwEventRegister(&ProviderGuid,
                          NULL,
                          NULL,
                          &ProviderHandle);
}

VOID
NTAPI
LogDestroy(
  VOID
  )
{
  EtwEventUnregister(ProviderHandle);
}

NTSTATUS
Log(
  CONST WCHAR* Format,
  ...
  )
{
  va_list Args;
  va_start(Args, Format);

  WCHAR Buffer[128];
  ntdll_vsnwprintf(Buffer,
                   RTL_NUMBER_OF(Buffer),
                   Format,
                   Args);

  va_end(Args);

  return EtwEventWriteString(ProviderHandle, 0, 0, Buffer);
}
