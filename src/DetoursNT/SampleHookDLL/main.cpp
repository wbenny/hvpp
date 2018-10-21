#include <windows.h>
#include <detours.h>

#define NtCurrentProcess()        ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess()        NtCurrentProcess()
#define NtCurrentThread()         ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread()         NtCurrentThread()

typedef NTSTATUS (NTAPI * fnNtTestAlert)(
  VOID
  );

static fnNtTestAlert OrigNtTestAlert;

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
NtTestAlert(
  VOID
  );

EXTERN_C
NTSTATUS
NTAPI
HookNtTestAlert(
  VOID
  )
{
  return 0x1337;
}

EXTERN_C
BOOL
WINAPI
NtDllMain(
  _In_ HINSTANCE hModule,
  _In_ DWORD dwReason,
  _In_ LPVOID lpvReserved
  )
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      OrigNtTestAlert = NtTestAlert;

      DetourTransactionBegin();
      DetourUpdateThread(NtCurrentThread());
      DetourAttach((PVOID*)&OrigNtTestAlert, HookNtTestAlert);
      DetourTransactionCommit();
      break;

    case DLL_PROCESS_DETACH:
      DetourTransactionBegin();
      DetourUpdateThread(NtCurrentThread());
      DetourDetach((PVOID*)&OrigNtTestAlert, HookNtTestAlert);
      DetourTransactionCommit();
      break;

    case DLL_THREAD_ATTACH:

      break;

    case DLL_THREAD_DETACH:

      break;
  }

  return TRUE;
}

