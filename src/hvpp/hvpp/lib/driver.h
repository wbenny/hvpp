#pragma once
#include "error.h"

namespace driver
{
  extern void* begin_address;
  extern void* end_address;

  extern void* kernel_begin_address;
  extern void* kernel_end_address;

  extern void* highest_user_address;
  extern void* system_range_start_address;

  namespace common
  {
    using driver_initialize_fn = error_code_t(*)();
    using driver_destroy_fn = void(*)();

    auto initialize(
      driver_initialize_fn driver_initialize = nullptr,
      driver_destroy_fn driver_destroy = nullptr
      ) noexcept -> error_code_t;

    void destroy() noexcept;

    auto hypervisor_allocator_recommended_capacity() noexcept -> size_t;

    auto system_allocator_default_initialize() noexcept -> error_code_t;
    void system_allocator_default_destroy() noexcept;

    auto hypervisor_allocator_default_initialize() noexcept -> error_code_t;
    void hypervisor_allocator_default_destroy() noexcept;
  }

  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;
}
