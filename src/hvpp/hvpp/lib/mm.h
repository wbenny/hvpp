#pragma once
#include "hvpp/ia32/memory.h"

#include "error.h"

#include "mm/paging_descriptor.h"
#include "mm/physical_memory_descriptor.h"
#include "mm/mtrr_descriptor.h"

#include <cstdint>

namespace mm
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
      allocator_guard(const allocator_guard& other) noexcept = delete;
      allocator_guard(allocator_guard&& other) noexcept = delete;
      allocator_guard(const allocator_t& new_allocator) noexcept;
      ~allocator_guard() noexcept;

      allocator_guard& operator=(const allocator_guard& other) noexcept = delete;
      allocator_guard& operator=(allocator_guard&& other) noexcept = delete;

    private:
      allocator_t previous_allocator_;
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

  auto paging_descriptor() noexcept -> const paging_descriptor_t&;
  auto physical_memory_descriptor() noexcept -> const physical_memory_descriptor_t&;
  auto mtrr_descriptor() noexcept -> const mtrr_descriptor_t&;
}
