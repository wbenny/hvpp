#include "../log.h"

#include "lib/mp.h"

#include <algorithm> // std::size

#include <cstring>

extern "C"
{
  void logger_detail_do_print(const char* message);
  void logger_detail_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
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
      char buffer[512];

      logger_detail_vsnprintf(buffer, sizeof(buffer), format, args);
      do_print(buffer);
    }
  }
}
