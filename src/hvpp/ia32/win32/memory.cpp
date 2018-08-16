#include "memory.h"
#include "ia32/memory.h"

#include <ntddk.h>

namespace ia32 {

  namespace detail
  {
    uint64_t pa_from_va(void* va) noexcept
    {
      return MmGetPhysicalAddress(va).QuadPart;
    }

    void* va_from_pa(uint64_t pa) noexcept
    {
      PHYSICAL_ADDRESS win_pa;
      win_pa.QuadPart = pa;

      return MmGetVirtualForPhysical(win_pa);
    }
  }

void physical_memory_descriptor::check_physical_memory() noexcept
{
  auto physical_memory_ranges = MmGetPhysicalMemoryRanges();

  do
  {
    pa_t address = physical_memory_ranges[count_].BaseAddress.QuadPart;
    size_t     size = physical_memory_ranges[count_].NumberOfBytes.QuadPart;

    if (!address && !size)
    {
      break;
    }

    range_[count_] = memory_range(address, address + size);
  } while (++count_ < max_range_count);
}

}
