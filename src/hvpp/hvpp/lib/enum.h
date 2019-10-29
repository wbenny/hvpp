#pragma once
#include <type_traits>

//
// (ref: http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/)
//

template<
  typename Enum1,
  typename Enum2 = Enum1
>
struct enum_operators
{
  static constexpr bool enable_neg = false;
  static constexpr bool enable_not = false;
  static constexpr bool enable_and = false;
  static constexpr bool enable_or  = false;
  static constexpr bool enable_xor = false;
};

#define hvpp_enum_operators(Enum1)            \
  template<>                                  \
  struct enum_operators<Enum1>                \
  {                                           \
    static constexpr bool enable_neg = true;  \
    static constexpr bool enable_not = true;  \
    static constexpr bool enable_and = true;  \
    static constexpr bool enable_or  = true;  \
    static constexpr bool enable_xor = true;  \
  }

template<typename Enum1>
constexpr inline typename std::enable_if_t<enum_operators<Enum1>::enable_neg, bool> operator!(Enum1 rhs) noexcept
{ return static_cast<bool>(!static_cast<std::underlying_type_t<Enum1>>(rhs)); }

template<typename Enum1>
constexpr inline typename std::enable_if_t<enum_operators<Enum1>::enable_not, Enum1> operator~(Enum1 rhs) noexcept
{ return static_cast<Enum1>(~static_cast<std::underlying_type_t<Enum1>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_and, Enum1> operator&(Enum1 lhs, Enum2 rhs) noexcept
{ return static_cast<Enum1>( static_cast<std::underlying_type_t<Enum1>>(lhs) & static_cast<std::underlying_type_t<Enum2>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_or, Enum1> operator|(Enum1 lhs, Enum2 rhs) noexcept
{ return static_cast<Enum1>(static_cast<std::underlying_type_t<Enum1>>(lhs) | static_cast<std::underlying_type_t<Enum2>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_xor, Enum1> operator^(Enum1 lhs, Enum2 rhs) noexcept
{ return static_cast<Enum1>(static_cast<std::underlying_type_t<Enum1>>(lhs) ^ static_cast<std::underlying_type_t<Enum2>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_and, Enum1>& operator&=(Enum1& lhs, Enum2 rhs) noexcept
{ return reinterpret_cast<Enum1&>(reinterpret_cast<std::underlying_type_t<Enum1>&>(lhs) &= static_cast<std::underlying_type_t<Enum2>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_or, Enum1>& operator|=(Enum1& lhs, Enum2 rhs) noexcept
{ return reinterpret_cast<Enum1&>(reinterpret_cast<std::underlying_type_t<Enum1>&>(lhs) |= static_cast<std::underlying_type_t<Enum2>>(rhs)); }

template<typename Enum1, typename Enum2>
constexpr inline typename std::enable_if_t<enum_operators<Enum1, Enum2>::enable_xor, Enum1>& operator^=(Enum1& lhs, Enum2 rhs) noexcept
{ return reinterpret_cast<Enum1&>(reinterpret_cast<std::underlying_type_t<Enum1>&>(lhs) ^= static_cast<std::underlying_type_t<Enum2>>(rhs)); }
