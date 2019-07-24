#include <ntddk.h>

#define HVPP_MAPPING_TAG 'mpvh'

namespace mm::detail
{
  void* mapper_allocate(size_t size) noexcept
  {
    return MmAllocateMappingAddress(size, HVPP_MAPPING_TAG);
  }

  void mapper_free(void* va) noexcept
  {
    MmFreeMappingAddress(va, HVPP_MAPPING_TAG);
  }
}
