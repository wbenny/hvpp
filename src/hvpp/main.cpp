#include "lib/mm.h"
#include "lib/assert.h"
#include "lib/log.h"

#include "hvpp/hypervisor.h"

#include "custom_vmexit.h"

#include <ntddk.h>

//////////////////////////////////////////////////////////////////////////
// Function prototypes.
//////////////////////////////////////////////////////////////////////////

template <
  typename TVmExitHandler
>
NTSTATUS
HvppInitialize(
  _Out_ hvpp::hypervisor** Hypervisor,
  _Out_ hvpp::vmexit_handler** VmExitHandler,
  _In_ PVOID Memory,
  _In_ SIZE_T Size
  );

VOID
HvppDestroy(
  _In_ hvpp::hypervisor* Hypervisor,
  _In_ hvpp::vmexit_handler* VmExitHandler
  );

//////////////////////////////////////////////////////////////////////////
// Variables.
//////////////////////////////////////////////////////////////////////////

extern "C" DRIVER_INITIALIZE DriverEntry;

static PVOID                  HvppMemory        = nullptr;
static SIZE_T                 HvppMemorySize    = 256 * 1024 * 1024;
static ULONG                  HvppMemoryTag     = 'ppvh';
static hvpp::hypervisor*      HvppHypervisor    = nullptr;
static hvpp::vmexit_handler*  HvppVmExitHandler = nullptr;

//////////////////////////////////////////////////////////////////////////
// Function implementations.
//////////////////////////////////////////////////////////////////////////

template <
  typename TVmExitHandler
>
NTSTATUS
HvppInitialize(
  _Out_ hvpp::hypervisor** Hypervisor,
  _Out_ hvpp::vmexit_handler** VmExitHandler,
  _In_ PVOID Memory,
  _In_ SIZE_T Size
  )
{
  hvpp::hypervisor* HypervisorInstance = nullptr;
  hvpp::vmexit_handler* VmExitHandlerInstance = nullptr;

  logger::initialize();
  memory_manager::initialize(Memory, Size);

  HypervisorInstance = new hvpp::hypervisor();

  if (!HypervisorInstance)
  {
    HvppDestroy(HypervisorInstance, VmExitHandlerInstance);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  if (!HypervisorInstance->check())
  {
    HvppDestroy(HypervisorInstance, VmExitHandlerInstance);
    return STATUS_HV_FEATURE_UNAVAILABLE;
  }

  VmExitHandlerInstance = new TVmExitHandler();
  if (!VmExitHandlerInstance)
  {
    HvppDestroy(HypervisorInstance, VmExitHandlerInstance);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  *Hypervisor = HypervisorInstance;
  *VmExitHandler = VmExitHandlerInstance;

  return STATUS_SUCCESS;
}

VOID
HvppDestroy(
  _In_ hvpp::hypervisor* Hypervisor,
  _In_ hvpp::vmexit_handler* VmExitHandler
  )
{
  if (Hypervisor)
  {
    delete Hypervisor;
  }

  if (VmExitHandler)
  {
    delete VmExitHandler;
  }

  memory_manager::destroy();
  logger::destroy();
}

extern "C"
VOID
DriverUnload(
  _In_ PDRIVER_OBJECT DriverObject
  )
{
  UNREFERENCED_PARAMETER(DriverObject);

  HvppHypervisor->stop();
  HvppDestroy(HvppHypervisor, HvppVmExitHandler);

  ExFreePoolWithTag(HvppMemory, HvppMemoryTag);
}

extern "C"
NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  NTSTATUS Status;

  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = &DriverUnload;

  HvppMemory = ExAllocatePoolWithTag(
    NonPagedPool,
    HvppMemorySize,
    HvppMemoryTag);

  if (!HvppMemory)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto Exit;
  }

  Status = HvppInitialize<custom_vmexit_handler>(
    &HvppHypervisor,
    &HvppVmExitHandler,
    HvppMemory,
    HvppMemorySize);

  if (!NT_SUCCESS(Status))
  {
    goto Exit;
  }

  HvppHypervisor->start(HvppVmExitHandler);

Exit:
  return Status;
}

