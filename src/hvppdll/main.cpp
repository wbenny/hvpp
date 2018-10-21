#include <ntdll_windows.h>
#include <ntdll.h>

#include "crt.h"
#include "log.h"
#include "device.h"
#include "hook.h"

VOID
NTAPI
HideModuleFromPeb(
  PVOID ModuleHandle
  )
{
  PPEB Peb = NtCurrentPeb();
  PLIST_ENTRY ListEntry;

  for (ListEntry =   Peb->Ldr->InLoadOrderModuleList.Flink;
       ListEntry != &Peb->Ldr->InLoadOrderModuleList;
       ListEntry =   ListEntry->Flink)
  {
    PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    //
    // ModuleHandle is same as DLL base address.
    //

    if (LdrEntry->DllBase == ModuleHandle)
    {
      RemoveEntryList(&LdrEntry->InLoadOrderLinks);
      RemoveEntryList(&LdrEntry->InInitializationOrderLinks);
      RemoveEntryList(&LdrEntry->InMemoryOrderLinks);
      RemoveEntryList(&LdrEntry->HashLinks);

      break;
    }
  }
}

NTSTATUS
NTAPI
OnProcessAttach(
  _In_ PVOID ModuleHandle
  )
{
  //
  // Initialize CRT.
  //

  CrtInitialize();

  //
  // Initialize ETW.
  //

  LogInitialize();

  // Log(Peb->ProcessParameters->CommandLine.Buffer);

  //
  // Initialize device.
  //

  DeviceInitialize();

  //
  // Hook all functions.
  //

  HookInitialize();

  //
  // Make us unloadable (by FreeLibrary calls).
  //

  LdrAddRefDll(LDR_ADDREF_DLL_PIN, ModuleHandle);

  //
  // Hide this DLL from the PEB.
  //

  HideModuleFromPeb(ModuleHandle);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
OnProcessDetach(
  _In_ HANDLE ModuleHandle
  )
{
  //
  // Unhook all functions.
  //

  HookDestroy();

  //
  // Disconnect device.
  //

  DeviceDestroy();

  //
  // Unregister the ETW logger.
  //

  LogDestroy();

  //
  // Destroy CRT.
  //

  CrtDestroy();

  return STATUS_SUCCESS;
}

EXTERN_C
BOOL
WINAPI
NtDllMain(
  _In_ HANDLE ModuleHandle,
  _In_ ULONG Reason,
  _In_ LPVOID Reserved
  )
{
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      OnProcessAttach(ModuleHandle);
      break;

    case DLL_PROCESS_DETACH:
      OnProcessDetach(ModuleHandle);
      break;

    case DLL_THREAD_ATTACH:

      break;

    case DLL_THREAD_DETACH:

      break;
  }

  return TRUE;
}
