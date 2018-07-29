#include "hvpp/hypervisor.h"
#include "lib/mm.h"
#include "lib/assert.h"

#include <ntddk.h>
#include <thread>

extern "C" DRIVER_INITIALIZE DriverEntry;

//
// 32MB of memory should be enough.
//
static size_t            hv_memory_size = 32 * 1024 * 1024;
static void*             hv_memory      = nullptr;
static uint32_t          hv_memory_tag  = 'ppvh';
static hvpp::hypervisor* hv             = nullptr;

extern "C"
VOID
DriverUnload(
  _In_ PDRIVER_OBJECT DriverObject
  )
{
  UNREFERENCED_PARAMETER(DriverObject);

  hv->stop();
  delete hv;

  logger::destroy();
  memory_manager::destroy();

  ExFreePoolWithTag(hv_memory, hv_memory_tag);
}

extern "C"
NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = &DriverUnload;

  hv_memory = ExAllocatePoolWithTag(NonPagedPool, hv_memory_size, hv_memory_tag);

  hvpp_assert(hv_memory != nullptr);

  memory_manager::initialize(hv_memory, hv_memory_size);
  logger::initialize(PAGE_SIZE * 16);

  hv_log("memory_manager::allocated_bytes(): %u", memory_manager::allocated_bytes());
  hv_log("memory_manager::free_bytes():      %u", memory_manager::free_bytes());
  hv_log("starting hypervisor...");

  hv = new hvpp::hypervisor();
  hv->check();
  hv->start();

  hv_log("hypervisor started...");

  hv_log("memory_manager::allocated_bytes(): %u", memory_manager::allocated_bytes());
  hv_log("memory_manager::free_bytes():      %u", memory_manager::free_bytes());

  return STATUS_SUCCESS;
}
