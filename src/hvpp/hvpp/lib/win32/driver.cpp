#include "../driver.h"
#include "../device.h"

#include "../error.h"

#include <algorithm>

#include <ntddk.h>
#include <ntimage.h>

#ifdef min
# undef min
#endif

#ifdef max
# undef max
#endif

//
// Macro to extract access out of the device io control code
//
#define ACCESS_FROM_CTL_CODE(ctrlCode)  (((ULONG)(ctrlCode & 0x0000c000)) >> 14)

EXTERN_C
NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
  _In_ PVOID BaseOfImage
  );

EXTERN_C
NTSYSAPI
PVOID
NTAPI
RtlPcToFileHeader(
  _In_ PVOID PcValue,
  _Out_ PVOID* BaseOfImage
  );

extern "C"
{
  DRIVER_INITIALIZE DriverEntry;
  PDRIVER_OBJECT GlobalDriverObject = nullptr;
}

namespace driver
{
  void* begin_address               = nullptr;
  void* end_address                 = nullptr;

  void* kernel_begin_address        = nullptr;
  void* kernel_end_address          = nullptr;

  void* highest_user_address        = nullptr;
  void* system_range_start_address  = nullptr;
}

NTSTATUS
NTAPI
HvppErrorCodeToNtStatus(
  error_code_t error
  );

NTSTATUS
NTAPI
DriverDispatch(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp
  )
{
  PIO_STACK_LOCATION IoStackLocation;
  device* CppDeviceObject;

  IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
  CppDeviceObject = *static_cast<device**>(DeviceObject->DeviceExtension);

  error_code_t err;

  size_t BytesTransferred = 0;

  PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
  ULONG BufferLength;
  ULONG IoControlCode;

  switch (IoStackLocation->MajorFunction)
  {
    case IRP_MJ_CREATE:
      err = CppDeviceObject->on_create();
      break;

    case IRP_MJ_CLOSE:
      err = CppDeviceObject->on_close();
      break;

    case IRP_MJ_READ:
      //
      // TODO: Offset?
      //
      BufferLength = IoStackLocation->Parameters.Read.Length;
      err = CppDeviceObject->on_read(Buffer, BufferLength, BytesTransferred);
      break;

    case IRP_MJ_WRITE:
      //
      // TODO: Offset?
      //
      BufferLength = IoStackLocation->Parameters.Write.Length;
      err = CppDeviceObject->on_write(Buffer, BufferLength, BytesTransferred);
      break;

    case IRP_MJ_DEVICE_CONTROL:
      //
      // Capture the IOCTL code.
      //
      IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;

      //
      // Set buffer length as the biggest of these buffers.
      // Note that Irp->AssociatedIrp.SystemBuffer is guaranteed
      // to be as big as biggest buffer of these two.
      //
      BufferLength = std::max(
        IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
        IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength
        );

      //
      // Set size of the output buffer immediately.
      // Driver should be responsible for filling that buffer.
      //
      if (ACCESS_FROM_CTL_CODE(IoControlCode) & FILE_WRITE_ACCESS)
      {
        BytesTransferred = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
      }

      err = CppDeviceObject->on_ioctl(Buffer, BufferLength, IoControlCode);
      break;

    default:
      break;
  }

  //
  // Complete the I/O request.
  //
  Irp->IoStatus.Status = HvppErrorCodeToNtStatus(err);
  Irp->IoStatus.Information = BytesTransferred;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  //
  // Unless this value is STATUS_PENDING, it [the NTSTATUS value]
  // must match the value of Irp->IoStatus.Status set by the driver
  // that completed the IRP.
  // (ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/returning-status-from-dispatch-routines)
  //
  // Note that we're not doing any async-I/O, so we never return
  // STATUS_PENDING.  Just pass down whatever is in IoStatus.Status.
  //
  return Irp->IoStatus.Status;
}

EXTERN_C
VOID
NTAPI
DriverUnload(
  _In_ PDRIVER_OBJECT DriverObject
  )
{
  UNREFERENCED_PARAMETER(DriverObject);

  driver::common::destroy();
}

EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  UNREFERENCED_PARAMETER(RegistryPath);

  GlobalDriverObject = DriverObject;
  DriverObject->MajorFunction[IRP_MJ_CREATE]         = &DriverDispatch;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]          = &DriverDispatch;
  DriverObject->MajorFunction[IRP_MJ_READ]           = &DriverDispatch;
  DriverObject->MajorFunction[IRP_MJ_WRITE]          = &DriverDispatch;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &DriverDispatch;
  DriverObject->DriverUnload                         = &DriverUnload;

  //
  // Save the driver address range.
  //

  driver::begin_address = (PVOID)((ULONG_PTR)(DriverObject->DriverStart));
  driver::end_address   = (PVOID)((ULONG_PTR)(DriverObject->DriverStart) + DriverObject->DriverSize);

  //
  // Save the kernel address range.
  //

  PVOID  ImageBase = NULL;
  SIZE_T ImageSize = 0;

  RtlPcToFileHeader((PVOID)&RtlPcToFileHeader, &ImageBase);

  if (!ImageBase)
  {
    return STATUS_NOT_FOUND;
  }

  PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)(RtlImageNtHeader(ImageBase));

  if (!NtHeaders)
  {
    return STATUS_NOT_FOUND;
  }

  ImageSize = NtHeaders->OptionalHeader.SizeOfImage;

  driver::kernel_begin_address = (PVOID)((ULONG_PTR)(ImageBase));
  driver::kernel_end_address   = (PVOID)((ULONG_PTR)(ImageBase) + ImageSize);

  //
  // Save the highest user address and system range start address.
  //

  driver::highest_user_address        = MmHighestUserAddress;
  driver::system_range_start_address  = MmSystemRangeStart;

  auto err = driver::common::initialize(&driver::initialize,
                                        &driver::destroy);

  return HvppErrorCodeToNtStatus(err);
}
