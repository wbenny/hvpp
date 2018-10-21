#pragma once
#include <ntdll_windows.h>
#include <ntdll.h>

NTSTATUS
NTAPI
HookInitialize(
  VOID
  );

VOID
NTAPI
HookDestroy(
  VOID
  );
