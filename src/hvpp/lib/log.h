#pragma once
#include <cstdint>

#define hv_log(format, ...) ::logger::print(::logger::level_t::debug, __FUNCTION__, format, __VA_ARGS__)

namespace logger
{
  enum class level_t
  {
    debug_enabled = 0x10,
    info_enabled  = 0x20,
    warn_enabled  = 0x40,
    error_enabled = 0x80,

    debug = debug_enabled | info_enabled | warn_enabled | error_enabled,
    info  = info_enabled | warn_enabled | error_enabled,
    warn  = warn_enabled | error_enabled,
    error = error_enabled,

    default = debug,
  };

  enum class options_t : uint32_t
  {
    print_buffered            = 0x1,
    print_time                = 0x2,
    print_processor_number    = 0x4,
    print_function_name       = 0x8,
    print_function_full_name  = 0x10,
    print_error_on_fail       = 0x20,

    default = /*print_buffered |*/ print_time | print_processor_number | print_function_full_name | print_error_on_fail,
  };

  inline level_t operator&(level_t value1, level_t value2) noexcept
  { return static_cast<level_t>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

  inline level_t operator|(level_t value1, level_t value2) noexcept
  { return static_cast<level_t>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

  inline level_t& operator&=(level_t& value1, level_t value2) noexcept
  { value1 = value1 & value2; return value1; }

  inline level_t& operator|=(level_t& value1, level_t value2) noexcept
  { value1 = value1 | value2; return value1; }

  inline options_t operator&(options_t value1, options_t value2) noexcept
  { return static_cast<options_t>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

  inline options_t operator|(options_t value1, options_t value2) noexcept
  { return static_cast<options_t>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

  inline options_t& operator&=(options_t& value1, options_t value2) noexcept
  { value1 = value1 & value2; return value1; }

  inline options_t& operator|=(options_t& value1, options_t value2) noexcept
  { value1 = value1 | value2; return value1; }

  void initialize(size_t size) noexcept;
  void destroy() noexcept;

  auto get_options() noexcept -> options_t;
  void set_options(options_t opt) noexcept;

  auto get_level() noexcept -> level_t;
  void set_level(level_t level) noexcept;

  void print(level_t level, const char* function, const char* format, ...) noexcept;
}
