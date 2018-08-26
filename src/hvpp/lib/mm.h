#pragma once
#include "ia32/memory.h"
#include "ia32/mtrr.h"

namespace memory_manager
{
  void initialize() noexcept;
  void destroy() noexcept;
  void assign(void* address, size_t size) noexcept;

  void* allocate(size_t size) noexcept;
  void free(void* address) noexcept;

  size_t allocated_bytes() noexcept;
  size_t free_bytes() noexcept;

  const ia32::physical_memory_descriptor& physical_memory_descriptor() noexcept;
  const ia32::mtrr& mtrr() noexcept;
}
