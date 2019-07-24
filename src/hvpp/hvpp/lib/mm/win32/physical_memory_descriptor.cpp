#include "hvpp/ia32/memory.h"

#include <ntddk.h>

using namespace ia32;

namespace mm::detail
{
  void check_physical_memory(physical_memory_range* range_list, int range_list_size, int& count) noexcept
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

      range_list[count] = physical_memory_range(address, address + size);
    } while (++count < range_list_size);

    ExFreePool(physical_memory_ranges);
  }
}
