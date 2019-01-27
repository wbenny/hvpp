#pragma once
#include "error.h"

namespace driver
{
  extern void* begin_address;
  extern void* end_address;

  namespace common
  {
    using driver_initialize_fn = error_code_t(*)();
    using driver_destroy_fn = void(*)();

    auto initialize(
      driver_initialize_fn driver_initialize = nullptr,
      driver_destroy_fn driver_destroy = nullptr
      ) noexcept -> error_code_t;

    void destroy() noexcept;
  }

  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;
}
