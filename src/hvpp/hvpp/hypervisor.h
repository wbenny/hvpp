#pragma once
#include "vcpu.h"
#include "vmexit.h"

#include "lib/error.h"

namespace hvpp::hypervisor
{
  auto start(vmexit_handler& handler) noexcept -> error_code_t;
  void stop() noexcept;

  bool is_running() noexcept;
}
