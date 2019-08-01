#include <ntddk.h>

#define HVPP_MEMORY_TAG 'ppvh'

namespace mm::detail
{
  auto system_allocate(size_t size) noexcept -> void*
  {
    return ExAllocatePoolWithTag(NonPagedPool, size, HVPP_MEMORY_TAG);
  }

  auto system_allocate_aligned(size_t size, size_t alignment) noexcept -> void*
  {
    //
    // If NumberOfBytes is PAGE_SIZE or greater, a page-aligned buffer is allocated.
    // Memory allocations of PAGE_SIZE or less are allocated within a page and do not
    // cross page boundaries.  Memory allocations of less than PAGE_SIZE are not
    // necessarily page-aligned but are aligned to 8-byte boundaries in 32-bit systems
    // and to 16-byte boundaries in 64-bit systems.
    //
    // (ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nf-wdm-exallocatepoolwithtag)

    NT_ASSERT(
      alignment > 0 &&
      alignment <= 4096 &&
      !(alignment & (alignment - 1)) // is power of 2
    );

    if (size >= PAGE_SIZE || alignment <= sizeof(void*))
    {
      return system_allocate(size);
    }

    //
    // Force alignment to PAGE_SIZE.
    //
    return system_allocate(PAGE_SIZE);
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
