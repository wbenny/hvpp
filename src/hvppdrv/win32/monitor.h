#pragma once
#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
NTAPI
MonInitialize(
  _In_opt_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutineEx,
  _In_opt_ PCREATE_THREAD_NOTIFY_ROUTINE CreateThreadNotifyRoutine,
  _In_opt_ PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageNotifyRoutine
  );

VOID
NTAPI
MonDestroy(
  VOID
  );

#ifdef __cplusplus
}
#endif
