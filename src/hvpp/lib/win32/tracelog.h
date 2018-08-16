#pragma once
#include "../log.h"

//
// Windows TraceLogging API.
//

namespace logger::tracelog {

  namespace detail {
    void vprint(level_t level, const char* function, const char* format, va_list args) noexcept;
  }

  void initialize() noexcept;
  void destroy() noexcept;

}
