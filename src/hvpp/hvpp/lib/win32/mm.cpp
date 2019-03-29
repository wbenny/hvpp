#include "../mm.h"

#include <ntddk.h>

#define HVPP_MEMORY_TAG 'ppvh'

namespace mm::detail
{
  auto system_allocate(size_t size) noexcept -> void*
  {
    return ExAllocatePoolWithTag(NonPagedPool,
                                 size,
                                 HVPP_MEMORY_TAG);
  }

  void system_free(void* address) noexcept
  {
    //
    // ExFreePoolWithTag doesn't support freeing NULL.
    //
    if (address == nullptr)
    {
      return;
    }

    ExFreePoolWithTag(address, HVPP_MEMORY_TAG);
  }
}
