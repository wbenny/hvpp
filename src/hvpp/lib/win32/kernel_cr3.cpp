#include "kernel_cr3.h"

#include <ntddk.h>

#define PCID_KERNEL 0
#define PCID_USER   1

ia32::cr3_t kernel_cr3(ia32::cr3_t cr3) noexcept
{
  //
  // Since January 3rd, 2018, Microsoft has started to use PCID (process-context
  // identifier) as a mitigation against Meltdown vulnerability.
  //
  // Note that the CPU must support the PCID capability (CPUID[1].ECX[17])
  // and the PCID must be enabled via CR4.PCID[17]
  //
  // (ref: https://blogs.technet.microsoft.com/srd/2018/03/23/kva-shadow-mitigating-meltdown-on-windows/)
  //

  struct NT_KPROCESS
  {
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    ULONG_PTR DirectoryTableBase;
    UCHAR Data[1];
  };

  //
  // Check if the VM-exit has been performed in kernel mode or user mode.
  //
  if (cr3.pcid == PCID_KERNEL)
  {
    //
    // If PCID == 0, the CR3 should already contain such PML4, which allows
    // us to access kernel space.
    //
    return cr3;
  }

  //
  // The VM-exit comes from user mode - user mode CR3 doesn't have mapped
  // the kernel space in the virtual address space. If we would switched
  // into the user mode CR3, we would get instantaneous #GP (as the user
  // mode doesn't have privilege to execute instructions from kernel mode
  // where we're currently executing. We will fetch the "real" CR3 of the
  // kernel from the PsGetCurrentProcess()->DirectoryTableBase field.
  //

  auto kprocess = reinterpret_cast<NT_KPROCESS*>(PsGetCurrentProcess());
  return ia32::cr3_t{ kprocess->DirectoryTableBase };
}
