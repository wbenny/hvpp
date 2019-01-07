#include <ntddk.h>
#include "hvpp.h"

#include "hypervisor.h"
#include "vcpu.h"
#include "lib/cr3_guard.h"
#include "lib/driver.h"
#include "lib/mm.h"
#include "vmexit/vmexit_c_wrapper.h"

#include <stdarg.h>

using namespace ia32;
using namespace hvpp;

#define hvpp_                                                 ((hypervisor*)Hvpp)
#define vcpu_                                                 ((vcpu_t*)Vcpu)
#define ept_                                                  ((ept_t*)Ept)

extern "C" {

static
NTSTATUS
NTAPI
ErrorCodeToNtStatus(
  error_code_t error
  )
{
  //
  // TODO: Something meaningful...
  //
  return !error
    ? STATUS_SUCCESS
    : STATUS_UNSUCCESSFUL;
}

//////////////////////////////////////////////////////////////////////////
// ept.h
//////////////////////////////////////////////////////////////////////////

#pragma region ept.h

PEPTE
NTAPI
HvppEptMap(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  )
{
  return (PEPTE)
    ept_->map(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart },
    (epte_t::access_type)Access
    );
}

PEPTE
NTAPI
HvppEptMapEx(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access,
  _In_ ULONG Large
  )
{
  return (PEPTE)
    ept_->map(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart },
    (epte_t::access_type)Access,
    (pml)Large
    );
}

PEPTE
NTAPI
HvppEptMap4Kb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  )
{
  return (PEPTE)
    ept_->map_4kb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart },
    (epte_t::access_type)Access
    );
}

PEPTE
NTAPI
HvppEptMap2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  )
{
  return (PEPTE)
    ept_->map_2mb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart },
    (epte_t::access_type)Access
    );
}

PEPTE
NTAPI
HvppEptMap1Gb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  )
{
  return (PEPTE)
    ept_->map_1gb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart },
    (epte_t::access_type)Access
    );
}

VOID
NTAPI
HvppEptSplit1GbTo2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  )
{
  ept_->split_1gb_to_2mb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart }
    );
}

VOID
NTAPI
HvppEptSplit2MbTo4Kb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  )
{
  ept_->split_2mb_to_4kb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart }
    );
}

VOID
NTAPI
HvppEptJoin2MbTo1Gb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  )
{
  ept_->join_2mb_to_1gb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart }
    );
}

VOID
NTAPI
HvppEptJoin4KbTo2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  )
{
  ept_->join_4kb_to_2mb(
    pa_t{ (uint64_t)GuestPhysicalAddress.QuadPart },
    pa_t{ (uint64_t)HostPhysicalAddress.QuadPart }
    );
}

EPT_PTR
NTAPI
HvppEptGetEptPointer(
  _In_ PEPT Ept
  )
{
  return EPT_PTR { ept_->ept_pointer().flags };
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// hypervisor.h
//////////////////////////////////////////////////////////////////////////

#pragma region hypervisor.h

NTSTATUS
NTAPI
HvppInitialize(
  _Out_ PHVPP* Hvpp
  )
{
  //
  // Initialize the memory manager and logger.
  //

  driver::common::initialize();

  //
  // Allocate memory for the hypervisor instance.
  //

  *Hvpp = (PHVPP)new hypervisor();

  if (!*Hvpp)
  {
    //
    // Allocation failed - exit.
    //

    driver::common::destroy();
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  //
  // Initialize the hypervisor.
  //

  return ErrorCodeToNtStatus(((hypervisor*)(*Hvpp))->initialize());
}

VOID
NTAPI
HvppDestroy(
  _In_ PHVPP Hvpp
  )
{
  //
  // Destroy the hypervisor.
  //

  hvpp_->destroy();
  delete &hvpp_->exit_handler();
  delete  hvpp_;

  //
  // Destroy the memory manager and logger.
  //

  driver::common::destroy();
}

NTSTATUS
NTAPI
HvppStart(
  _In_ PHVPP Hvpp,
  _In_ PVMEXIT_HANDLER VmExitHandler
  )
{
  //
  // Create the VM-exit handler instance.
  //

  auto exit_handler = new vmexit_c_wrapper_handler();

  //
  // Initialize the C-handlers array.
  //

  vmexit_c_wrapper_handler::c_handler_array_t c_handlers;
  memcpy(c_handlers.data(), VmExitHandler->HandlerRoutine, sizeof(VmExitHandler->HandlerRoutine));

  //
  // Initialize the VM-exit handler.
  //

  exit_handler->initialize(c_handlers);

  //
  // Start the hypervisor.
  //

  return ErrorCodeToNtStatus(hvpp_->start(*exit_handler));
}

VOID
NTAPI
HvppStop(
  _In_ PHVPP Hvpp
  )
{
  hvpp_->stop();
}

BOOLEAN
NTAPI
HvppIsStarted(
  _In_ PHVPP Hvpp
  )
{
  return hvpp_->is_started();
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// vcpu.h
//////////////////////////////////////////////////////////////////////////

#pragma region vcpu.h

PEPT
NTAPI
HvppVcpuGetEpt(
  _In_ PVCPU Vcpu
  )
{
  return (PEPT)&vcpu_->ept();
}

PVCPU_CONTEXT
NTAPI
HvppVcpuExitContext(
  _In_ PVCPU Vcpu
  )
{
  return (PVCPU_CONTEXT)&vcpu_->exit_context();
}

VOID
NTAPI
HvppVcpuSuppressRipAdjust(
  _In_ PVCPU Vcpu
  )
{
  vcpu_->suppress_rip_adjust();
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////

#pragma region Helpers

PVOID
NTAPI
HvppAllocate(
  ULONG Size
  )
{
  return new uint8_t[Size];
}

VOID
NTAPI
HvppFree(
  PVOID Address
  )
{
  delete[] (uint8_t*)Address;
}

ULONG64
NTAPI
HvppVmRead(
  _In_ VMCS_FIELD VmcsField
  )
{
  ULONG64 Result = 0;
  vmx::vmread((vmx::vmcs_t::field)VmcsField, Result);
  return Result;
}

VOID
NTAPI
HvppVmWrite(
  _In_ VMCS_FIELD VmcsField,
  _In_ ULONG64 VmcsValue
  )
{
  vmx::vmwrite((vmx::vmcs_t::field)VmcsField, VmcsValue);
}

ULONG_PTR
NTAPI
HvppVmCall(
  _In_ ULONG_PTR Rcx,
  _In_ ULONG_PTR Rdx,
  _In_ ULONG_PTR R8,
  _In_ ULONG_PTR R9
  )
{
  return vmx::vmcall(Rcx, Rdx, R8, R9);
}

VOID
NTAPI
HvppInveptAll(
  VOID
  )
{
  vmx::invept_all_contexts();
}

VOID
NTAPI
HvppInveptSingleContext(
  _In_ EPT_PTR EptPointer
  )
{
  vmx::invept_single_context(ept_ptr_t{ EptPointer.Flags });
}

VOID
NTAPI
HvppAttachAddressSpace(
  _Inout_ ULONG_PTR* Cr3
  )
{
  ULONG_PTR NewCr3 = *Cr3;
  ULONG_PTR PreviousCr3 = ia32::read<ia32::cr3_t>().flags;

  ia32::write<ia32::cr3_t>(::detail::kernel_cr3(ia32::cr3_t{ NewCr3 }));

  *Cr3 = PreviousCr3;
}

VOID
NTAPI
HvppDetachAddressSpace(
  _In_ ULONG_PTR Cr3
  )
{
  ia32::write<ia32::cr3_t>(ia32::cr3_t{ Cr3 });
}

VOID
HvppTrace(
  _In_ const CHAR* Format,
  ...
  )
{
  va_list Args;
  va_start(Args, Format);

  logger::detail::vprint_trace(logger::level_t::trace, "__UNKNOWN__", Format, Args);

  va_end(Args);
}

#pragma endregion

}
