#include "../cr3_guard.h"

#include <ntddk.h>

#define PCID_NONE   0x000
#define PCID_USER   0x001 // KeMakeUserDirectoryTableBase
#define PCID_KERNEL 0x002 // KeMakeKernelDirectoryTableBase

namespace detail {

ia32::cr3_t kernel_cr3(ia32::cr3_t cr3) noexcept
{
  //
  // Since January 3rd, 2018, Microsoft has started to use
  // PCID (process-context identifier) as a mitigation against
  // Meltdown vulnerability.
  //
  // Note that the CPU must support the PCID capability
  // (CPUID[1].ECX[17]) and the PCID must be enabled via
  // CR4.PCIDE[17].
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

  if (cr3.pcid == PCID_NONE)
  {
    //
    // PCID is not in use - we can just return the original
    // CR3.  When PCID is not in use, CR3 is same for both
    // user-mode and kernel-mode.
    //
    // Note that this check relies on totally undocumented
    // Windows internals.  More correct would be to check
    // if CR4.PCIDE[17] == 0.
    //

    return cr3;
  }

#if 0
  if (cr3.pcid == PCID_KERNEL)
  {
    //
    // CR3 already contains such PML4, that allows us to
    // access kernel-mode address space.  We can just reuse
    // it.
    //
    // Note that this check relies on totally undocumented
    // Windows internals.  If Microsoft decides to change
    // the PCID_KERNEL value, we can end up in trouble.
    //

    return cr3;
  }
#endif

  //
  // If the VM-exit comes from user-mode, then the guest CR3
  // doesn't have mapped kernel-mode address space.  If we
  // would've switched into that (user-mode) CR3, we would've
  // get #GP instantaneously because the user-mode doesn't have
  // privilege to execute instructions from kernel-mode where
  // we're currently executing.  We will fetch the "real" CR3
  // of the kernel from the PsGetCurrentProcess()->DirectoryTableBase
  // field.
  //
  // If the VM-exit comes from kernel-mode, then the guest CR3 should
  // be the same CR3 as in the PsGetCurrentProcess()->DirectoryTableBase
  // field.
  //
  // Note that although this still relies on undocumented KPROCESS
  // structure fields, the offset of the "DirectoryTableBase" field
  // never changed, so let's be confident and just use it.
  //
#if 0
  hvpp_assert(cr3.pcid == PCID_USER);
#endif

  const auto kprocess = reinterpret_cast<NT_KPROCESS*>(PsGetCurrentProcess());
  return ia32::cr3_t{ kprocess->DirectoryTableBase };
}

}
