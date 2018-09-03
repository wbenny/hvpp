#include "log.h"

#include <cstdarg>
#include <cstdio>

//
// Simple logger implementation.
// There are two types of log operations:
//   - standard log
//   - trace log
//
// Standard log has levels "debug", "info", "warn", "error".
// Trace log has level "trace".
//
// Standard logs are always printed to the debugger.
// Trace logs use tracing API, which on Windows can be observed e.g. with
// traceview.exe util (found in WDK).
//
// Main benefit of tracing compared to regular "debug prints" is that they
// can be called at VERY HIGH frequency (more than 10000 per sec.) and
// on Windows they can be called from any IRQL.
//

namespace logger
{
  level_t   current_level   = level_t::default_flags;
  options_t current_options = options_t::default_flags;

  auto initialize() noexcept -> error_code_t
  { return detail::initialize(); }

  void destroy() noexcept
  { detail::destroy(); }

  void set_options(options_t options) noexcept
  { current_options = options; }

  auto get_options() noexcept -> options_t
  { return current_options; }

  bool test_options(options_t options) noexcept
  { return (current_options & options) == options; }

  void set_level(level_t level) noexcept
  { current_level = level; }

  auto get_level() noexcept -> level_t
  { return current_level; }

  bool test_level(level_t level) noexcept
  { return (current_level & level) == level; }

  void print(level_t level, const char* function, const char* format, ...) noexcept
  {
    va_list args;
    va_start(args, format);

    if (level == level_t::trace)
    {
      detail::vprint_trace(level, function, format, args);
    }
    else
    {
      detail::vprint      (level, function, format, args);
    }

    va_end(args);
  }
}
