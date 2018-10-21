#include "device.h"
#include "../hvpp/hvpp/lib/ioctl.h"

#include <ntdll_windows.h>
#include <ntdll.h>

using ioctl_monitor_start_t     = ioctl_read_t<1, sizeof(uint64_t)>;
using ioctl_monitor_stop_t      = ioctl_none_t<1>;

using ioctl_hypervisor_start_t  = ioctl_none_t<2>;
using ioctl_hypervisor_stop_t   = ioctl_none_t<3>;

struct ioctl_shadow_page_add_params
{
  uint64_t read_write;
  uint64_t execute;
  uint64_t offset;
};

using ioctl_shadow_page_add_t   = ioctl_read_t<5, sizeof(ioctl_shadow_page_add_params)>;
using ioctl_shadow_page_apply_t = ioctl_none_t<6>;

#define IOCTL_SHADOW_PAGE_ADD    (ioctl_shadow_page_add_t::code())
#define IOCTL_SHADOW_PAGE_APPLY  (ioctl_shadow_page_apply_t::code())

HANDLE DeviceHandle = INVALID_HANDLE_VALUE;

NTSTATUS
NTAPI
DeviceInitialize(
  VOID
  )
{
  UNICODE_STRING ObjectName;
  RtlInitUnicodeString(&ObjectName, (PWSTR)L"\\??\\hvpp");

  OBJECT_ATTRIBUTES ObjectAttributes;
  InitializeObjectAttributes(&ObjectAttributes,
                             &ObjectName,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL);

  IO_STATUS_BLOCK IoStatusBlock;
  return NtCreateFile(&DeviceHandle,
                      GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      NULL,
                      FILE_ATTRIBUTE_NORMAL,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      FILE_OPEN,
                      FILE_SYNCHRONOUS_IO_NONALERT,
                      NULL,
                      0);
}

VOID
NTAPI
DeviceDestroy(
  VOID
  )
{
  if (DeviceHandle != INVALID_HANDLE_VALUE)
  {
    NtClose(DeviceHandle);
  }
}

BOOLEAN
DeviceIsOpen(
  VOID
  )
{
  return DeviceHandle != INVALID_HANDLE_VALUE;
}

NTSTATUS
NTAPI
DeviceIoctl(
  ULONG ControlCode,
  PVOID Buffer,
  ULONG BufferSize
  )
{
  if (!DeviceIsOpen())
  {
    return STATUS_DEVICE_NOT_CONNECTED;
  }

  IO_STATUS_BLOCK IoStatusBlock;
  return NtDeviceIoControlFile(DeviceHandle,
                               NULL,
                               NULL,
                               NULL,
                               &IoStatusBlock,
                               ControlCode,
                               Buffer,
                               BufferSize,
                               Buffer,
                               BufferSize);
}

NTSTATUS
NTAPI
DeviceShadowPageAdd(
  PVOID ReadWrite,
  PVOID Execute,
  ULONG Offset
  )
{
  ioctl_shadow_page_add_params Parameters{
    (uint64_t)ReadWrite,
    (uint64_t)Execute,
    (uint64_t)Offset
  };

  return DeviceIoctl(IOCTL_SHADOW_PAGE_ADD, &Parameters, sizeof(Parameters));
}

NTSTATUS
NTAPI
DeviceShadowPageApply(
  VOID
  )
{
  return DeviceIoctl(IOCTL_SHADOW_PAGE_APPLY, NULL, 0);
}

