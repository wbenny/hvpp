#pragma once
#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
NTAPI
TraceInitialize(
  VOID
  );

VOID
NTAPI
TraceDestroy(
  VOID
  );

VOID
NTAPI
Trace(
  _In_ PWCHAR Message
  );

VOID
TraceFormat(
  _In_ const char* Format,
  ...
  );

#ifdef __cplusplus
}
#endif
