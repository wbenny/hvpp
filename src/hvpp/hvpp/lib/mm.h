#pragma once
#include "hvpp/ia32/memory.h"
#include "hvpp/ia32/mtrr.h"

#include "error.h"

#include <cstdint>

namespace memory_manager
{
  namespace detail
  {
    auto system_allocate(size_t size) noexcept -> void*;
    void system_free(void* address) noexcept;
  }

  using allocate_fn_t = void*(*)(size_t);
  using free_fn_t     = void(*)(void*);

  struct allocator_t
  {
    allocate_fn_t allocate;
    free_fn_t     free;
  };

  class allocator_guard
  {
    public:
      allocator_guard() noexcept;
      allocator_guard(const allocator_t& new_allocator) noexcept;
      ~allocator_guard() noexcept;

    private:
      const allocator_t& previous_allocator_;
  };

  extern const allocator_t system_allocator;
  extern const allocator_t custom_allocator;

  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;

  auto assign(void* address, size_t size) noexcept -> error_code_t;

  auto allocate(size_t size) noexcept -> void*;
  void free(void* address) noexcept;

  auto system_allocate(size_t size) noexcept -> void*;
  void system_free(void* address) noexcept;

  auto allocated_bytes() noexcept -> size_t;
  auto free_bytes() noexcept -> size_t;

  auto allocator() noexcept -> const allocator_t&;
  void allocator(const allocator_t& new_allocator) noexcept;

  auto physical_memory_descriptor() noexcept -> const ia32::physical_memory_descriptor&;
  auto mtrr() noexcept -> const ia32::mtrr&;
}
