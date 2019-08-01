#include "system_memory_allocator.h"

namespace mm
{
  namespace detail
  {
    auto system_allocate(size_t size) noexcept -> void*;
    auto system_allocate_aligned(size_t size, size_t alignment) noexcept -> void*;
    void system_free(void* address) noexcept;
  }

  auto system_memory_allocator::attach(void* address, size_t size) noexcept -> error_code_t
  {
    (void)(address);
    (void)(size);
    return make_error_code_t(std::errc::not_supported);
  }

  void system_memory_allocator::detach() noexcept
  {
    return;
  }

  auto system_memory_allocator::allocate(size_t size) noexcept -> void*
  {
    return detail::system_allocate(size);
  }

  auto system_memory_allocator::allocate_aligned(size_t size, size_t alignment) noexcept -> void*
  {
    return detail::system_allocate_aligned(size, alignment);
  }

  void system_memory_allocator::free(void* address) noexcept
  {
    detail::system_free(address);
  }

  bool system_memory_allocator::contains(void* address) noexcept
  {
    (void)(address);
    return true;
  }

  auto system_memory_allocator::allocated_bytes() noexcept -> size_t
  {
    return 0;
  }

  auto system_memory_allocator::free_bytes() noexcept -> size_t
  {
    return 0;
  }
}
