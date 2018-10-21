#include "monitor.h"

//
// VOID
// NTAPI
// MonCreateProcessNotifyRoutineEx(
//   _Inout_ PEPROCESS Process,
//   _In_ HANDLE ProcessId,
//   _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
//   )
// {
//   UNREFERENCED_PARAMETER(Process);
//   UNREFERENCED_PARAMETER(ProcessId);
//   UNREFERENCED_PARAMETER(CreateInfo);
// }
//
// VOID
// NTAPI
// MonCreateThreadNotifyRoutine(
//   _In_ HANDLE ProcessId,
//   _In_ HANDLE ThreadId,
//   _In_ BOOLEAN Create
//   )
// {
//   UNREFERENCED_PARAMETER(ProcessId);
//   UNREFERENCED_PARAMETER(ThreadId);
//   UNREFERENCED_PARAMETER(Create);
// }
//
// VOID
// NTAPI
// MonLoadImageNotifyRoutine(
//   _In_opt_ PUNICODE_STRING FullImageName,
//   _In_ HANDLE ProcessId,
//   _In_ PIMAGE_INFO ImageInfo
//   )
// {
//   UNREFERENCED_PARAMETER(FullImageName);
//   UNREFERENCED_PARAMETER(ProcessId);
//   UNREFERENCED_PARAMETER(ImageInfo);
// }
//

PCREATE_PROCESS_NOTIFY_ROUTINE_EX MonCreateProcessNotifyRoutineEx = NULL;
PCREATE_THREAD_NOTIFY_ROUTINE     MonCreateThreadNotifyRoutine    = NULL;
PLOAD_IMAGE_NOTIFY_ROUTINE        MonLoadImageNotifyRoutine       = NULL;

NTSTATUS
NTAPI
MonInitialize(
  _In_opt_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutineEx,
  _In_opt_ PCREATE_THREAD_NOTIFY_ROUTINE CreateThreadNotifyRoutine,
  _In_opt_ PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageNotifyRoutine
  )
{
  NTSTATUS Status = STATUS_SUCCESS;

  //
  // CreateProcessNotifyRoutineEx
  //

  if (CreateProcessNotifyRoutineEx)
  {
    Status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutineEx, FALSE);

    if (!NT_SUCCESS(Status))
    {
      goto Error;
    }

    MonCreateProcessNotifyRoutineEx = CreateProcessNotifyRoutineEx;
  }

  //
  // CreateThreadNotifyRoutine
  //

  if (CreateThreadNotifyRoutine)
  {
    Status = PsSetCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);

    if (!NT_SUCCESS(Status))
    {
      goto Error;
    }

    MonCreateThreadNotifyRoutine = CreateThreadNotifyRoutine;
  }

  //
  // LoadImageNotifyRoutine
  //

  if (LoadImageNotifyRoutine)
  {
    Status = PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);

    if (!NT_SUCCESS(Status))
    {
      goto Error;
    }

    MonLoadImageNotifyRoutine = LoadImageNotifyRoutine;
  }

  goto Exit;

  //
  // Error handling.
  //

Error:
  MonDestroy();

Exit:
  return Status;
}

VOID
NTAPI
MonDestroy(
  VOID
  )
{
  if (MonLoadImageNotifyRoutine)
  {
    PsRemoveLoadImageNotifyRoutine(MonLoadImageNotifyRoutine);
  }

  if (MonCreateThreadNotifyRoutine)
  {
    PsRemoveCreateThreadNotifyRoutine(MonCreateThreadNotifyRoutine);
  }

  if (MonCreateProcessNotifyRoutineEx)
  {
    PsSetCreateProcessNotifyRoutineEx(MonCreateProcessNotifyRoutineEx, TRUE);
  }
}
