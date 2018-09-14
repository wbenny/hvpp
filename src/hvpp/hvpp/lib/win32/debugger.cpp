#include "../debugger.h"

#include <ntddk.h>

namespace debugger::detail
{
  bool is_enabled() noexcept
  {
    return KD_DEBUGGER_ENABLED;
  }
}
