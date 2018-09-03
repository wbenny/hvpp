#include "../driver.h"

#include "lib/error.h"

#include <ntddk.h>

EXTERN_C DRIVER_INITIALIZE DriverEntry;

NTSTATUS
ErrorCodeToNtStatus(
  error_code_t error
  )
{
  if (!error)
  {
    return STATUS_SUCCESS;
  }

  return STATUS_UNSUCCESSFUL;
}

EXTERN_C
VOID
DriverUnload(
  _In_ PDRIVER_OBJECT DriverObject
  )
{
  UNREFERENCED_PARAMETER(DriverObject);

  driver::common::destroy();
}

EXTERN_C
NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = &DriverUnload;

  auto err = driver::common::initialize();

  return ErrorCodeToNtStatus(err);
}
