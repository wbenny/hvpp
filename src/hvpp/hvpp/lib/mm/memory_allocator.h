#pragma once
#include <cstddef>    // size_t

#include "../error.h"

namespace mm
{
  class memory_allocator
  {
    public:
      memory_allocator() noexcept = default;
      memory_allocator(const memory_allocator& other) noexcept = delete;
      memory_allocator(memory_allocator&& other) noexcept = delete;
      memory_allocator& operator=(const memory_allocator& other) noexcept = delete;
      memory_allocator& operator=(memory_allocator&& other) noexcept = delete;
      virtual ~memory_allocator() noexcept = default;

      virtual auto attach(void* address, size_t size) noexcept -> error_code_t = 0;
      virtual void detach() noexcept = 0;

      virtual auto allocate(size_t size) noexcept -> void* = 0;
      virtual auto allocate_aligned(size_t size, size_t alignment) noexcept -> void* = 0;
      virtual void free(void* address) noexcept = 0;

      virtual bool contains(void* address) noexcept = 0;

      virtual auto allocated_bytes() noexcept -> size_t = 0;
      virtual auto free_bytes() noexcept -> size_t = 0;
  };
}
