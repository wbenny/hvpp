#include "../log.h"

#include "lib/mp.h"

#include <algorithm> // std::size

#include <cstring>

extern "C"
{
  void logger_detail_do_print(const char* message);
}

namespace logger::detail
{
  void do_print(const char* message) noexcept
  {
    logger_detail_do_print(message);
  }

  void vprint(level_t level, const char* function, const char* format, va_list args) noexcept
  {
    if (test_level(level))
    {
      do_print(format);
    }
  }
}
