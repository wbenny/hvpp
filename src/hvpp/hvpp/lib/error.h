#pragma once
#include <system_error>
#include <type_traits>

//
// Simple reimplementation of error_code from <system_error>
// without message() and category() methods.  Because message()
// method returns std::string which relies on several internal CRT
// functions and because category() relies on thread-safe initialization
// via std::call_once, I've decided it'll be best to just reimplement
// that class without those methods instead of mimicking internal
// CRT functions.
//
// I am so, so sorry for this...
//

class error_code_t
{
  public:
    constexpr error_code_t() noexcept
      : value_{ 0 }
    { }

    constexpr error_code_t(int value) noexcept
      : value_{ value }
    { }

    template<
      class EnumT,
      std::enable_if_t<std::is_error_code_enum_v<EnumT>, int> = 0
    >
    constexpr error_code_t(EnumT value) noexcept
      : value_{ (int)value }
    { }

    template<
      class EnumT,
      std::enable_if_t<std::is_error_code_enum_v<EnumT>, int> = 0
    >
    constexpr error_code_t& operator=(EnumT value) noexcept
    { value_ = (int)value; return *this; }

    constexpr void assign(int value) noexcept
    { value_ = value; }

    constexpr void clear() noexcept
    { value_ = 0; }

    constexpr int value() const noexcept
    { return value_; }

    constexpr explicit operator bool() const noexcept
    { return value() != 0; }

  private:
    int value_;
};

constexpr inline error_code_t make_error_code_t(std::errc value) noexcept
{ return error_code_t((int)value); }
