#pragma once
#include <cstdint>

#include "win32/mp.h"

//
// Multi-Processor functions.
//

namespace mp {

inline uint32_t cpu_index() noexcept
{
  return detail::cpu_index();
}

inline void sleep(uint32_t milliseconds) noexcept
{
  detail::sleep(milliseconds);
}

template <typename T>
inline void ipi_call(T* instance, void (T::*member_function)() noexcept) noexcept
{
  //
  // Inter-Processor Interrupt - runs specified method on all logical CPUs.
  //
  struct ipi_ctx
  {
    T* instance;
    void (T::*member_function)() noexcept;
  } ipi_context {
    instance,
    member_function
  };

  detail::ipi_call([](void* context) noexcept {
    auto ipi_context = reinterpret_cast<ipi_ctx*>(context);
    auto instance = ipi_context->instance;
    auto member_function = ipi_context->member_function;

    (instance->*member_function)();
  }, &ipi_context);
}

}
