#include "../mm.h"

#include <ntddk.h>

#define HVPP_MEMORY_TAG 'ppvh'

namespace memory_manager::detail
{
  void* system_allocate(size_t size) noexcept
  {
    return ExAllocatePoolWithTag(NonPagedPool,
                                 size,
                                 HVPP_MEMORY_TAG);
  }

  void system_free(void* address) noexcept
  {
    ExFreePoolWithTag(address, HVPP_MEMORY_TAG);
  }
}
