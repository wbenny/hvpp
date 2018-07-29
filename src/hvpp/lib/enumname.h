#pragma once
#include <cstdint>

// struct enum_name
// {
//   const char* name;
//   uint64_t    value;
// 
//   template <typename T, size_t N>
//   static const char* get(const enum_name(&name_array)[N], T value) noexcept
//   {
//     for (auto& item : name_array)
//     {
//       if (static_cast<T>(item.value) == value)
//       {
//         return item.name;
//       }
//     }
// 
//     return nullptr;
//   }
// };
