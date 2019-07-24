#pragma once
#include "hvpp/ia32/asm.h"

namespace debugger
{
  namespace detail
  {
    bool is_enabled() noexcept;
  }

  inline void breakpoint() noexcept
  { ia32_asm_int3(); }

  inline bool is_enabled() noexcept
  { return detail::is_enabled(); }

  inline void breakpoint_if_enabled() noexcept
  { if (is_enabled()) breakpoint(); }
}
