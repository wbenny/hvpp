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

  inline void ipi_call(void(*callback)(void*), void* context) noexcept
  { detail::ipi_call(callback, context); }

  inline void ipi_call(void(*callback)()) noexcept
  { detail::ipi_call([](void* context) { ((void(*)())context)(); }, callback); }

  template <
    typename T
  >
  inline void ipi_call(T* instance, void (T::*member_function)()) noexcept
  {
    //
    // Inter-Processor Interrupt - runs specified method on all logical CPUs.
    //
    struct ipi_ctx
    {
      T* instance;
      void (T::*member_function)();
    } ipi_context {
      instance,
      member_function
    };

    ipi_call([](void* context) noexcept {
      auto ipi_context = reinterpret_cast<ipi_ctx*>(context);
      auto instance = ipi_context->instance;
      auto member_function = ipi_context->member_function;

      (instance->*member_function)();
    }, &ipi_context);
  }
}
