#pragma once
#include "error.h"

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
