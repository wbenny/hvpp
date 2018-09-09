#include "../mp.h"

#include <cstdint>

extern "C"
{
  uint32_t mp_detail_cpu_count(void);
  uint32_t mp_detail_cpu_index(void);
  void     mp_detail_sleep(uint32_t milliseconds);
  void     mp_detail_ipi_call(void(*callback)(void*) noexcept, void* context);
}

namespace mp::detail
{
  uint32_t cpu_count() noexcept
  {
    return mp_detail_cpu_count();
  }

  uint32_t cpu_index() noexcept
  {
    return mp_detail_cpu_index();
  }

  void sleep(uint32_t milliseconds) noexcept
  {
    mp_detail_sleep(milliseconds);
  }

  void ipi_call(void(*callback)(void*) noexcept, void* context) noexcept
  {
    mp_detail_ipi_call(callback, context);
  }
}
