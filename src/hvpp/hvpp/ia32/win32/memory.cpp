#include "../memory.h"

#include <ntddk.h>

#define HVPP_MAPPING_TAG 'mpvh'

namespace ia32::detail
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

  void* mapping_allocate(size_t size) noexcept
  {
    return MmAllocateMappingAddress(size, HVPP_MAPPING_TAG);
  }

  void mapping_free(void* va) noexcept
  {
    MmFreeMappingAddress(va, HVPP_MAPPING_TAG);
  }

  void check_physical_memory(memory_range* range_list, int range_list_size, int& count) noexcept
  {
    auto physical_memory_ranges = MmGetPhysicalMemoryRanges();

    count = 0;

    do
    {
      pa_t   address = physical_memory_ranges[count].BaseAddress.QuadPart;
      size_t size    = physical_memory_ranges[count].NumberOfBytes.QuadPart;

      if (!address && !size)
      {
        break;
      }

      range_list[count] = memory_range(address, address + size);
    } while (++count < range_list_size);
	
    ExFreePool(physical_memory_ranges);
  }
}
