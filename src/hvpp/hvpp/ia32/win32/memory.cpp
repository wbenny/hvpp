#include "../memory.h"

#include <ntddk.h>

namespace ia32::detail
{
  uint64_t pa_from_va(const void* va) noexcept
  {
    return MmGetPhysicalAddress((PVOID)(va)).QuadPart;
  }

  void* va_from_pa(uint64_t pa) noexcept
  {
    PHYSICAL_ADDRESS win_pa;
    win_pa.QuadPart = pa;

    return MmGetVirtualForPhysical(win_pa);
  }
}
