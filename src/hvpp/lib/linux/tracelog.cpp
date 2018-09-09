#include "../log.h"

namespace logger::detail
{
  auto initialize() noexcept -> error_code_t
  {
    return error_code_t{};
  }

  void destroy() noexcept
  {

  }

  void vprint_trace(level_t level, const char* function, const char* format, va_list args) noexcept
  {

  }
}
