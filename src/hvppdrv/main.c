#include <ntddk.h>
#include <hvpp/hvpp.h>

VOID
NTAPI
HvppHandleExecuteCpuid(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  )
{
  PVCPU_CONTEXT Context = HvppVcpuExitContext(Vcpu);

  if (Context->Eax == 'ppvh')
  {
    Context->Rax = 'lleh';
    Context->Rbx = 'rf o';
    Context->Rcx = 'h mo';
    Context->Rdx = 'ppv';
  }
  else
  {
    HvppVmExitPassthrough(Passthrough);
  }
}

VOID
NTAPI
HvppHandleExecuteVmcall(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  )
{
  UNREFERENCED_PARAMETER(Vcpu);

  HvppVmExitPassthrough(Passthrough);
}

VOID
NTAPI
HvppHandleEptViolation(
  _In_ PVCPU Vcpu,
  _In_ PVMEXIT_PASSTHROUGH Passthrough
  )
{
  UNREFERENCED_PARAMETER(Passthrough);

  VMX_EXIT_QUALIFICATION_EPT_VIOLATION EptViolation;
  PHYSICAL_ADDRESS GuestPhysicalAddress;
  PVOID GuestLinearAddress;
  PEPT Ept;

  EptViolation.Flags            =           HvppVmRead(VMCS_VMEXIT_QUALIFICATION);
  GuestPhysicalAddress.QuadPart = (LONGLONG)HvppVmRead(VMCS_VMEXIT_GUEST_PHYSICAL_ADDRESS);
  GuestLinearAddress            = (PVOID)   HvppVmRead(VMCS_VMEXIT_GUEST_LINEAR_ADDRESS);

  Ept = HvppVcpuGetEpt(Vcpu);

  if (EptViolation.DataRead || EptViolation.DataWrite)
  {
    HvppEptMap4Kb(Ept, GuestPhysicalAddress, GuestPhysicalAddress, EPT_ACCESS_READ_WRITE);
  }
  else if (EptViolation.DataExecute)
  {
    HvppEptMap4Kb(Ept, GuestPhysicalAddress, GuestPhysicalAddress, EPT_ACCESS_EXECUTE);
  }

  HvppVcpuSuppressRipAdjust(Vcpu);
}

EXTERN_C
NTSTATUS
DriverEntry(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PUNICODE_STRING RegistryPath
  )
{
  UNREFERENCED_PARAMETER(DriverObject);
  UNREFERENCED_PARAMETER(RegistryPath);

  PHVPP Hypervisor;
  HvppInitialize(&Hypervisor);

  VMEXIT_HANDLER VmExitHandler = { 0 };
  VmExitHandler.HandlerRoutine[VMEXIT_REASON_EXECUTE_CPUID] = &HvppHandleExecuteCpuid;
  VmExitHandler.HandlerRoutine[VMEXIT_REASON_EXECUTE_VMCALL] = &HvppHandleExecuteVmcall;
  VmExitHandler.HandlerRoutine[VMEXIT_REASON_EPT_VIOLATION] = &HvppHandleEptViolation;
  HvppStart(Hypervisor, &VmExitHandler);

  return STATUS_SUCCESS;
}
