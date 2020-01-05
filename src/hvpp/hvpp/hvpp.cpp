#include <ntddk.h>
#include "hvpp.h"

#include "hypervisor.h"
#include "vcpu.h"
#include "lib/assert.h"
#include "lib/cr3_guard.h"
#include "lib/driver.h"
#include "lib/mm.h"
#include "vmexit/vmexit_c_wrapper.h"

#include <stdarg.h>

using namespace ia32;
using namespace hvpp;

#define vcpu_                                                 ((vcpu_t*)Vcpu)
#define ept_                                                  ((ept_t*)Ept)

NTSTATUS
NTAPI
HvppErrorCodeToNtStatus(
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

error_code_t
NTAPI
HvppNtStatusToErrorCode(
  NTSTATUS Status
  )
{
  if (!NT_SUCCESS(Status))
  {
    return make_error_code_t(std::errc::operation_not_supported);
  }

  return {};
}

extern "C" {

//////////////////////////////////////////////////////////////////////////
// ept.h
//////////////////////////////////////////////////////////////////////////

#pragma region ept.h

PEPT
NTAPI
HvppEptCreate(
  VOID
  )
{
  return (PEPT)(new ept_t{});
}

VOID
NTAPI
HvppEptDestroy(
  _In_ PEPT Ept
  )
{
  delete ept_;
}

VOID
NTAPI
HvppEptMapIdentity(
  _In_ PEPT Ept
  )
{
  ept_->map_identity();
}

VOID
NTAPI
HvppEptMapIdentityEx(
  _In_ PEPT Ept,
  _In_ ULONG Access
  )
{
  ept_->map_identity((epte_t::access_type)(Access));
}

VOID
NTAPI
HvppEptMapIdentitySparse(
  _In_ PEPT Ept
  )
{
  ept_->map_identity_sparse();
}

VOID
NTAPI
HvppEptMapIdentitySparseEx(
  _In_ PEPT Ept,
  _In_ ULONG Access
  )
{
  ept_->map_identity_sparse((epte_t::access_type)(Access));
}

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
      pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
      pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) },
      (epte_t::access_type)(Access)
      );
}

PEPTE
NTAPI
HvppEptMapEx(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access,
  _In_ ULONG Level
  )
{
  return (PEPTE)
    ept_->map(
      pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
      pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) },
      (epte_t::access_type)(Access),
      (pml)(Level)
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
      pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
      pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) },
      (epte_t::access_type)(Access)
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
      pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
      pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) },
      (epte_t::access_type)(Access)
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
      pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
      pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) },
      (epte_t::access_type)(Access)
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
    pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
    pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) }
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
    pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
    pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) }
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
    pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
    pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) }
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
    pa_t{ (uint64_t)(GuestPhysicalAddress.QuadPart) },
    pa_t{ (uint64_t)(HostPhysicalAddress.QuadPart) }
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

static vmexit_c_wrapper_handler* c_exit_handler = nullptr;

NTSTATUS
NTAPI
HvppInitialize(
  VOID
  )
{
  //
  // Initialize the memory manager and logger.
  //

  if (auto err = driver::common::initialize())
  {
    driver::common::destroy();
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  if (auto err = driver::common::system_allocator_default_initialize())
  {
    driver::common::destroy();
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  if (auto err = driver::common::hypervisor_allocator_default_initialize())
  {
    driver::common::system_allocator_default_destroy();
    driver::common::destroy();
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  return STATUS_SUCCESS;
}

VOID
NTAPI
HvppDestroy(
  VOID
  )
{
  //
  // Destroy the memory manager and logger.
  //

  driver::common::destroy();
  driver::common::hypervisor_allocator_default_destroy();
  driver::common::system_allocator_default_destroy();
}

NTSTATUS
NTAPI
HvppStart(
  _In_ PVMEXIT_HANDLER VmExitHandler,
  _In_ PVMEXIT_HANDLER_SETUP_ROUTINE SetupRoutine,
  _In_ PVMEXIT_HANDLER_TEARDOWN_ROUTINE TeardownRoutine,
  _In_ PVMEXIT_HANDLER_TERMINATE_ROUTINE TerminateRoutine
  )
{
  //
  // Initialize the C-handlers array.
  //

  vmexit_c_wrapper_handler::c_handler_array_t c_handlers;
  memcpy(c_handlers.data(), VmExitHandler->HandlerRoutine, sizeof(VmExitHandler->HandlerRoutine));

  //
  // Create the VM-exit handler instance.
  //

  hvpp_assert(c_exit_handler == nullptr);
  c_exit_handler = new vmexit_c_wrapper_handler(c_handlers, SetupRoutine, TeardownRoutine, TerminateRoutine, NULL);

  //
  // Start the hypervisor.
  //

  return HvppErrorCodeToNtStatus(hypervisor::start(*c_exit_handler));
}

VOID
NTAPI
HvppStop(
  VOID
  )
{
  hvpp_assert(c_exit_handler != nullptr);

  hypervisor::stop();

  delete c_exit_handler;
}

BOOLEAN
NTAPI
HvppIsRunning(
  VOID
  )
{
  return hypervisor::is_running();
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// vcpu.h
//////////////////////////////////////////////////////////////////////////

#pragma region vcpu.h

VOID
NTAPI
HvppVcpuEnableEpt(
  _In_ PVCPU Vcpu
  )
{
  vcpu_->ept_enable();
}

VOID
NTAPI
HvppVcpuDisableEpt(
  _In_ PVCPU Vcpu
  )
{
  vcpu_->ept_disable();
}

PEPT
NTAPI
HvppVcpuGetEpt(
  _In_ PVCPU Vcpu
  )
{
  return (PEPT)(&vcpu_->ept());
}

VOID
NTAPI
HvppVcpuSetEpt(
  _In_ PVCPU Vcpu,
  _In_ PEPT Ept
  )
{
  vcpu_->ept(*(ept_t*)(Ept));
}

PVCPU_CONTEXT
NTAPI
HvppVcpuContext(
  _In_ PVCPU Vcpu
  )
{
  return (PVCPU_CONTEXT)(&vcpu_->context());
}

VOID
NTAPI
HvppVcpuSuppressRipAdjust(
  _In_ PVCPU Vcpu
  )
{
  vcpu_->suppress_rip_adjust();
}

PVOID
NTAPI
HvppVcpuGetUserData(
  _In_ PVCPU Vcpu
  )
{
  return vcpu_->user_data();
}

VOID
NTAPI
HvppVcpuSetUserData(
  _In_ PVCPU Vcpu,
  _In_ PVOID UserData
  )
{
  vcpu_->user_data(UserData);
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
  delete[] (uint8_t*)(Address);
}

ULONG64
NTAPI
HvppVmRead(
  _In_ VMCS_FIELD VmcsField
  )
{
  ULONG64 Result = 0;
  vmx::vmread((vmx::vmcs_t::field)(VmcsField), Result);
  return Result;
}

VOID
NTAPI
HvppVmWrite(
  _In_ VMCS_FIELD VmcsField,
  _In_ ULONG64 VmcsValue
  )
{
  vmx::vmwrite((vmx::vmcs_t::field)(VmcsField), VmcsValue);
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
  return vmx::vmcall_fast(Rcx, Rdx, R8, R9);
}

ULONG_PTR
NTAPI
HvppVmCallEx(
  _In_ ULONG_PTR Rcx,
  _In_ ULONG_PTR Rdx,
  _In_ ULONG_PTR R8,
  _In_ ULONG_PTR R9,
  _In_ ULONG_PTR R10,
  _In_ ULONG_PTR R11,
  _In_ ULONG_PTR R12,
  _In_ ULONG_PTR R13,
  _In_ ULONG_PTR R14,
  _In_ ULONG_PTR R15
  )
{
  return vmx::vmcall_slow(Rcx, Rdx, R8, R9, R10, R11, R12, R13, R14, R15);
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
