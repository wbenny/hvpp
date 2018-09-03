#pragma once
#include "lib/error.h"

namespace driver
{
  namespace common
  {
    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;
  }

  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;
}
