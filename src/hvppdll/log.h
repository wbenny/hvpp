#pragma once
#include <ntdll_windows.h>
#include <ntdll.h>

NTSTATUS
NTAPI
LogInitialize(
  VOID
  );

VOID
NTAPI
LogDestroy(
  VOID
  );

NTSTATUS
Log(
  CONST WCHAR* Format,
  ...
  );
