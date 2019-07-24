#pragma once
#include "asm.h"

#include <cstdint>
#include <type_traits>

namespace ia32::msr {

template <typename T, typename = void> struct has_msr_id_t : std::false_type { };
template <typename T>                  struct has_msr_id_t<T, decltype(T::msr_id, void())> : std::true_type { };
template <typename T>          constexpr bool has_msr_id_v = has_msr_id_t<T>::value;

template <typename T> inline auto     read()                                 noexcept { return typename T::result_type { ia32_asm_read_msr(T::msr_id) }; }
template <typename T> inline T        read(uint32_t msr_id)                  noexcept { return          T              { ia32_asm_read_msr(   msr_id) }; }
                      inline uint64_t read(uint32_t msr_id)                  noexcept { return                           ia32_asm_read_msr(   msr_id)  ; }

template <typename T> inline void     write(T r)                             noexcept { ia32_asm_write_msr(T::msr_id, r.flags)   ; }
template <typename T> inline void     write(uint32_t msr_id, T r)            noexcept { ia32_asm_write_msr(   msr_id, r.flags)   ; }
                      inline void     write(uint32_t msr_id, uint64_t value) noexcept { ia32_asm_write_msr(   msr_id, value)     ; }

}
