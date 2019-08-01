#pragma once
#include "hvpp/ia32/memory.h"

#include "error.h"

#include "mm/memory_allocator.h"
#include "mm/memory_allocator/system_memory_allocator.h"
#include "mm/memory_allocator/hypervisor_memory_allocator.h"
#include "mm/paging_descriptor.h"
#include "mm/physical_memory_descriptor.h"
#include "mm/mtrr_descriptor.h"

#include <cstdint>

namespace mm
{
  //
  // Initialize & destroy.
  //
  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;

  //
  // System allocator.
  //
  auto system_allocator() noexcept -> memory_allocator*;
  void system_allocator(memory_allocator* new_allocator) noexcept;

  //
  // Hypervisor allocator.
  //
  auto hypervisor_allocator() noexcept -> memory_allocator*;
  void hypervisor_allocator(memory_allocator* new_allocator) noexcept;

  //
  // Current allocator.
  //
  auto allocator() noexcept -> memory_allocator*;
  void allocator(memory_allocator* new_allocator) noexcept;

  //
  // Descriptor getters.
  //
  auto paging_descriptor() noexcept -> const paging_descriptor_t&;
  auto physical_memory_descriptor() noexcept -> const physical_memory_descriptor_t&;
  auto mtrr_descriptor() noexcept -> const mtrr_descriptor_t&;

  //
  // Allocator guard.
  //
  class allocator_guard
  {
    public:
      allocator_guard() noexcept;
      allocator_guard(const allocator_guard& other) noexcept = delete;
      allocator_guard(allocator_guard&& other) noexcept = delete;
      allocator_guard(memory_allocator* new_allocator) noexcept;
      ~allocator_guard() noexcept;

      allocator_guard& operator=(allocator_guard& other) noexcept = delete;
      allocator_guard& operator=(allocator_guard&& other) noexcept = delete;

    private:
      memory_allocator* previous_allocator_;
  };
}
