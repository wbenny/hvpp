#pragma once
#include <cstdint>

//
// Multi-Processor functions.
//

namespace mp
{
  namespace detail
  {
    uint32_t cpu_count() noexcept;
    uint32_t cpu_index() noexcept;
    void     sleep(uint32_t milliseconds) noexcept;
    void     ipi_call(void(*callback)(void*), void* context) noexcept;
  }

  inline uint32_t cpu_count() noexcept
  { return detail::cpu_count(); }

  inline uint32_t cpu_index() noexcept
  { return detail::cpu_index(); }

  inline void sleep(uint32_t milliseconds) noexcept
  { detail::sleep(milliseconds); }

  //
  // Inter-Processor Interrupt - runs specified method on all logical CPUs.
  //

  inline void ipi_call(void(*callback)(void*), void* context) noexcept
  { detail::ipi_call(callback, context); }

  inline void ipi_call(void(*callback)()) noexcept
  { detail::ipi_call([](void* context) { ((void(*)())(context))(); }, callback); }

  template <typename T>
  inline void ipi_call(T function) noexcept
  { ipi_call([](void* context) noexcept { ((T*)(context))->operator()(); }, &function); }
}
