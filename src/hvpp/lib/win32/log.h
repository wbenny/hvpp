#pragma once
#include "../log.h"

//
// Windows DbgPrint API.
//

namespace logger::detail
{
  void vprint(level_t level, const char* function, const char* format, va_list args) noexcept;
}
