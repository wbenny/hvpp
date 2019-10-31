#include <ntddk.h>
#include <hvpp/hvpp.h>

#include "vmexit_custom.h"

//////////////////////////////////////////////////////////////////////////
//
// hvppdrv, but rewritten to use C interface.
// Note that support for IOCTL is missing.
//
//////////////////////////////////////////////////////////////////////////

VOID
NTAPI
DriverUnload(
  _In_ PDRIVER_OBJECT DriverObject
  )
{
  UNREFERENCED_PARAMETER(DriverObject);

  HvppStop();
  HvppDestroy();
}

NTSTATUS
NTAPI
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  UNREFERENCED_PARAMETER(RegistryPath);

  NTSTATUS Status;

  DriverObject->DriverUnload = &DriverUnload;

  Status = HvppInitialize();

  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

  VMEXIT_HANDLER VmExitHandler = { {
    [VMEXIT_REASON_EXECUTE_CPUID]  = &HvppHandleExecuteCpuid,
    [VMEXIT_REASON_EXECUTE_VMCALL] = &HvppHandleExecuteVmcall,
    [VMEXIT_REASON_EPT_VIOLATION]  = &HvppHandleEptViolation,
  } };

  Status = HvppStart(&VmExitHandler, &HvppSetup, &HvppTeardown, NULL);

  if (!NT_SUCCESS(Status))
  {
    HvppDestroy();
    return Status;
  }

  return STATUS_SUCCESS;
}
