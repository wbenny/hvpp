#pragma once
#include <ntdll_windows.h>
#include <ntdll.h>

NTSTATUS
NTAPI
DeviceInitialize(
  VOID
  );

VOID
NTAPI
DeviceDestroy(
  VOID
  );

BOOLEAN
DeviceIsOpen(
  VOID
  );

NTSTATUS
NTAPI
DeviceIoctl(
  ULONG ControlCode,
  PVOID Buffer,
  ULONG BufferSize
  );

NTSTATUS
NTAPI
DeviceShadowPageAdd(
  PVOID ReadWrite,
  PVOID Execute,
  ULONG Offset
  );

NTSTATUS
NTAPI
DeviceShadowPageApply(
  VOID
  );

