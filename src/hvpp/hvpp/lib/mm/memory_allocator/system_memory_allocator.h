#pragma once
#include "../memory_allocator.h"

namespace mm
{
  class system_memory_allocator
    : public memory_allocator
  {
    public:
      auto attach(void* address, size_t size) noexcept -> error_code_t override;
      void detach() noexcept override;

      auto allocate(size_t size) noexcept -> void* override;
      auto allocate_aligned(size_t size, size_t alignment) noexcept -> void* override;
      void free(void* address) noexcept override;

      bool contains(void* address) noexcept override;

      auto allocated_bytes() noexcept -> size_t override;
      auto free_bytes() noexcept -> size_t override;
  };
}
