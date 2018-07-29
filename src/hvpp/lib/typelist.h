#pragma once
#include <utility>
#include <type_traits>

struct type_list_base
{
};

template <typename ...ARGS>
struct type_list : type_list_base
{
  static constexpr int length = sizeof...(ARGS);
};

template <class T>
using is_type_list = std::is_base_of<type_list_base, T>;

template <class T>
constexpr bool is_type_list_v = is_type_list<T>::value;

template <typename F, typename T>
void for_each_type_impl(T&&, F&&, int&&) noexcept
{
}

template <typename F, typename T, typename ...ARGS>
void for_each_type_impl(type_list<T, ARGS...>&&, F&& f, int&& index) noexcept
{
  if constexpr (!is_type_list_v<T>)
  {
    f(T{}, index++);
  }

  for_each_type_impl(T{}, std::forward<F>(f), std::move(index));
  for_each_type_impl(type_list<ARGS...>{}, std::forward<F>(f), std::move(index));
}

template <typename F, typename T, typename ...ARGS>
void for_each_type(type_list<T, ARGS...>&&, F&& f) noexcept
{
  for_each_type_impl(type_list<T, ARGS...>{}, std::forward<F>(f), 0);
}
