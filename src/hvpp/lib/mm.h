#pragma once

namespace memory_manager
{
  void initialize(void* base_address, size_t size) noexcept;
  void destroy() noexcept;

  void* allocate(size_t size) noexcept;
  void free(void* address) noexcept;

  size_t allocated_bytes() noexcept;
  size_t free_bytes() noexcept;
}

