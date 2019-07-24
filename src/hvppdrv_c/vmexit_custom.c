#include "vmexit_custom.h"

#pragma warning(disable : 4204)

#define EPT_PD_MASK                         (~((ULONG64)(0x200000 - 1)))
#define EPT_PD_PAGE_ALIGN(PhysicalAddress)                            \
  ((PHYSICAL_ADDRESS) {                                               \
    .QuadPart = ((PhysicalAddress).QuadPart & EPT_PD_MASK)            \
  })

typedef struct _PER_VCPU_DATA
{
  PHYSICAL_ADDRESS PageRead;
  PHYSICAL_ADDRESS PageExec;
} PER_VCPU_DATA, *PPER_VCPU_DATA;

PER_VCPU_DATA PerVcpuData[32];

VOID
NTAPI
HvppHandleExecuteCpuid(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  )
{
  PVCPU_CONTEXT Context = HvppVcpuContext(Vcpu);

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

  PVCPU_CONTEXT Context = HvppVcpuContext(Vcpu);
  PEPT Ept = HvppVcpuGetCurrentEpt(Vcpu);

  PPER_VCPU_DATA Data = &PerVcpuData[KeGetCurrentProcessorNumberEx(NULL)];

  switch (Context->Rcx)
  {
    case 0xC1:
      {
        ULONG_PTR Cr3;
        HvppAttachAddressSpace(&Cr3);
        Data->PageRead = MmGetPhysicalAddress(Context->RdxAsPointer);
        Data->PageExec = MmGetPhysicalAddress(Context->R8AsPointer);
        HvppDetachAddressSpace(Cr3);
      }

      HvppTrace("vmcall (hook) EXEC: 0x%p READ: 0x%p",
                Data->PageExec.QuadPart,
                Data->PageRead.QuadPart);

      HvppEptSplit2MbTo4Kb(Ept,
                           EPT_PD_PAGE_ALIGN(Data->PageExec),
                           EPT_PD_PAGE_ALIGN(Data->PageExec));

      HvppEptMap4Kb(Ept,
                    Data->PageExec,
                    Data->PageExec,
                    EPT_ACCESS_EXECUTE);

      HvppInveptSingleContext(HvppEptGetEptPointer(Ept));
      break;

    case 0xC2:
      HvppTrace("vmcall (unhook)");

      HvppEptJoin4KbTo2Mb(Ept,
                          EPT_PD_PAGE_ALIGN(Data->PageExec),
                          EPT_PD_PAGE_ALIGN(Data->PageExec));

      HvppInveptSingleContext(HvppEptGetEptPointer(Ept));
      break;

    default:
      HvppVmExitPassthrough(Passthrough);
      break;
  }
}

VOID
NTAPI
HvppHandleEptViolation(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  )
{
  UNREFERENCED_PARAMETER(Passthrough);

  VMX_EXIT_QUALIFICATION_EPT_VIOLATION EptViolation;
  PHYSICAL_ADDRESS GuestPhysicalAddress;
  PVOID GuestLinearAddress;

  EptViolation.Flags            =           HvppVmRead(VMCS_VMEXIT_QUALIFICATION);
  GuestPhysicalAddress.QuadPart = (LONGLONG)HvppVmRead(VMCS_VMEXIT_GUEST_PHYSICAL_ADDRESS);
  GuestLinearAddress            = (PVOID)   HvppVmRead(VMCS_VMEXIT_GUEST_LINEAR_ADDRESS);

  PEPT Ept = HvppVcpuGetCurrentEpt(Vcpu);

  PPER_VCPU_DATA Data = &PerVcpuData[KeGetCurrentProcessorNumberEx(NULL)];

  if (EptViolation.DataRead || EptViolation.DataWrite)
  {
    HvppTrace("data_read LA: 0x%p PA: 0x%p",
              GuestLinearAddress,
              GuestPhysicalAddress.QuadPart);

    HvppEptMap4Kb(Ept,
                  Data->PageExec,
                  Data->PageRead,
                  EPT_ACCESS_READ_WRITE);
  }
  else if (EptViolation.DataExecute)
  {
    HvppTrace("data_execute LA: 0x%p PA: 0x%p",
              GuestLinearAddress,
              GuestPhysicalAddress.QuadPart);

    HvppEptMap4Kb(Ept,
                  Data->PageExec,
                  Data->PageExec,
                  EPT_ACCESS_EXECUTE);
  }

  HvppVcpuSuppressRipAdjust(Vcpu);
}
