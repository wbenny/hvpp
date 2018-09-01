#pragma once
#include <cstdint>

namespace mp::detail
{
  uint32_t cpu_count() noexcept;
  uint32_t cpu_index() noexcept;
  void sleep(uint32_t milliseconds) noexcept;
  void ipi_call(void(*callback)(void*), void* context) noexcept;
}
