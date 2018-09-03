#pragma once
#include "lib/error.h"

#include <cstdint>

#define hvpp_trace(format, ...)  ::logger::print(::logger::level_t::trace, __FUNCTION__, format, __VA_ARGS__)
#define hvpp_debug(format, ...)  ::logger::print(::logger::level_t::debug, __FUNCTION__, format, __VA_ARGS__)
#define hvpp_info(format, ...)   ::logger::print(::logger::level_t::info,  __FUNCTION__, format, __VA_ARGS__)
#define hvpp_warn(format, ...)   ::logger::print(::logger::level_t::warn,  __FUNCTION__, format, __VA_ARGS__)
#define hvpp_error(format, ...)  ::logger::print(::logger::level_t::error, __FUNCTION__, format, __VA_ARGS__)

namespace logger
{
  enum class level_t : uint32_t
  {
    trace = 0x01,
    debug = 0x02,
    info  = 0x04,
    warn  = 0x08,
    error = 0x10,

#ifdef DEBUG
    default_flags = trace | debug | info | warn | error
#else
    default_flags = trace         | info | warn | error
#endif
  };

  enum class options_t : uint32_t
  {
    print_time                = 0x01,
    print_processor_number    = 0x02,
    print_function_name       = 0x04,

    default_flags = print_time | print_processor_number /*| print_function_name*/,
  };

  constexpr inline level_t operator&(level_t value1, level_t value2) noexcept
  { return static_cast<level_t>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

  constexpr inline level_t operator|(level_t value1, level_t value2) noexcept
  { return static_cast<level_t>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

  constexpr inline level_t& operator&=(level_t& value1, level_t value2) noexcept
  { value1 = value1 & value2; return value1; }

  constexpr inline level_t& operator|=(level_t& value1, level_t value2) noexcept
  { value1 = value1 | value2; return value1; }

  constexpr inline options_t operator&(options_t value1, options_t value2) noexcept
  { return static_cast<options_t>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

  constexpr inline options_t operator|(options_t value1, options_t value2) noexcept
  { return static_cast<options_t>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

  constexpr inline options_t& operator&=(options_t& value1, options_t value2) noexcept
  { value1 = value1 & value2; return value1; }

  constexpr inline options_t& operator|=(options_t& value1, options_t value2) noexcept
  { value1 = value1 | value2; return value1; }

  namespace detail
  {
    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;

    void vprint(level_t level, const char* function, const char* format, va_list args) noexcept;
    void vprint_trace(level_t level, const char* function, const char* format, va_list args) noexcept;
  }

  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;

  auto get_options() noexcept -> options_t;
  void set_options(options_t options) noexcept;
  bool test_options(options_t option) noexcept;

  auto get_level() noexcept -> level_t;
  void set_level(level_t level) noexcept;
  bool test_level(level_t level) noexcept;

  void print(level_t level, const char* function, const char* format, ...) noexcept;
}
