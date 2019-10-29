#pragma once
#include "asm.h"
#include "ept.h"
#include "memory.h"
#include "msr.h"

#include "msr/vmx.h"

#include "vmx/exit_qualification.h"
#include "vmx/exit_reason.h"
#include "vmx/interrupt.h"
#include "vmx/instruction_error.h"
#include "vmx/instruction_info.h"
#include "vmx/vmcs.h"
#include "vmx/exception_bitmap.h"
#include "vmx/io_bitmap.h"
#include "vmx/msr_bitmap.h"

#include <cstdint>

namespace ia32::vmx {

namespace detail {

template <typename T>
union u64_t
{
  static_assert(sizeof(T) <= sizeof(uint64_t));
  static_assert(std::is_trivial_v<T>);

  uint64_t as_uint64_t;
  T        as_value;
};

}

enum class error_code
{
  success            = 0,
  failed_with_status = 1,
  failed             = 2
};

template <typename T>
auto adjust(T desired) noexcept
{
  constexpr bool is_control_register =
    std::is_same_v<T, cr0_t>                      ||
    std::is_same_v<T, cr4_t>;

  constexpr bool is_dr6 =
    std::is_same_v<T, dr6_t>;

  constexpr bool is_dr7 =
    std::is_same_v<T, dr7_t>;

  constexpr bool is_vmx_ctl_msr =
    std::is_same_v<T, msr::vmx_pinbased_ctls_t>   ||
    std::is_same_v<T, msr::vmx_procbased_ctls_t>  ||
    std::is_same_v<T, msr::vmx_entry_ctls_t>      ||
    std::is_same_v<T, msr::vmx_exit_ctls_t>;

  constexpr bool is_vmx_procbased_ctls2_msr =
    std::is_same_v<T, msr::vmx_procbased_ctls2_t>;

  static_assert(is_control_register               ||
                is_dr6                            ||
                is_dr7                            ||
                is_vmx_ctl_msr                    ||
                is_vmx_procbased_ctls2_msr,
                "type is not adjustable");

  if constexpr (is_control_register)
  {
    using fixed0_msr_t = std::conditional_t<std::is_same_v<T, cr0_t>,
                                            msr::vmx_cr0_fixed0_t,
                                            msr::vmx_cr4_fixed0_t>;

    using fixed1_msr_t = std::conditional_t<std::is_same_v<T, cr0_t>,
                                            msr::vmx_cr0_fixed1_t,
                                            msr::vmx_cr4_fixed1_t>;

    auto cr_fixed0 = msr::read<fixed0_msr_t>();
    auto cr_fixed1 = msr::read<fixed1_msr_t>();

    desired.flags |= cr_fixed0.flags;
    desired.flags &= cr_fixed1.flags;
  }
  else if constexpr (is_dr6)
  {
    //
    // x |= ~x will set all bits of the bitfield to 1's.  It would be prettier
    // to just use ~0, but compilers would complain about value not fitting into
    // bitfield.
    //
    // Vol3B[17.2(Debug Registers)] describes which reserved fields should be
    // set to 0 and 1 respectively.
    //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
    desired.reserved_1 |= ~desired.reserved_1;
    desired.reserved_2 = 0;
    desired.reserved_3 |= ~desired.reserved_3;
#pragma GCC diagnostic pop
  }
  else if constexpr (is_dr7)
  {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
    desired.reserved_1 |= ~desired.reserved_1;
    desired.reserved_2 = 0;
    desired.reserved_3 = 0;
#pragma GCC diagnostic pop
  }
  else if constexpr (is_vmx_ctl_msr)
  {
    constexpr auto true_msr_id = T::msr_id + 0xC;
    auto true_ctls = msr::read<msr::vmx_true_ctls_t>(true_msr_id);

    desired.flags |= true_ctls.allowed_0_settings;
    desired.flags &= true_ctls.allowed_1_settings;
  }
  else if constexpr (is_vmx_procbased_ctls2_msr)
  {
    auto true_ctls = msr::read<msr::vmx_true_ctls_t>(T::msr_id);

    desired.flags |= true_ctls.allowed_0_settings;
    desired.flags &= true_ctls.allowed_1_settings;
  }

  return desired;
}

inline error_code on(pa_t pa) noexcept
{ return static_cast<error_code>(ia32_asm_vmx_on((uint64_t*)&pa)); }

inline void off() noexcept
{ ia32_asm_vmx_off(); }

inline error_code vmptrld(pa_t pa) noexcept
{ return static_cast<error_code>(ia32_asm_vmx_vmptr_write(reinterpret_cast<uint64_t*>(&pa))); }

inline void vmptrst(pa_t pa) noexcept
{ ia32_asm_vmx_vmptr_read(reinterpret_cast<uint64_t*>(&pa)); }

inline error_code vmclear(pa_t pa) noexcept
{ return static_cast<error_code>(ia32_asm_vmx_vmclear(reinterpret_cast<uint64_t*>(&pa))); }

inline error_code vmlaunch() noexcept
{ return static_cast<error_code>(ia32_asm_vmx_vmlaunch()); }

inline error_code vmresume() noexcept
{ return static_cast<error_code>(ia32_asm_vmx_vmresume()); }

template <typename T>
inline error_code vmread(vmcs_t::field vmcs_field, T& value) noexcept
{
  detail::u64_t<T> u{};

  auto result = static_cast<error_code>(ia32_asm_vmx_vmread(static_cast<uint64_t>(vmcs_field), &u.as_uint64_t));

  value = u.as_value;

  return result;
}

template <typename T>
inline error_code vmwrite(vmcs_t::field vmcs_field, T value) noexcept
{
  detail::u64_t<T> u{};

  u.as_value = value;

  return static_cast<error_code>(ia32_asm_vmx_vmwrite(static_cast<uint64_t>(vmcs_field), u.as_uint64_t));
}

template <
  typename TResult = uint64_t,
  typename TArg1   = uint64_t,
  typename TArg2   = uint64_t,
  typename TArg3   = uint64_t,
  typename TArg4   = uint64_t
>
inline TResult vmcall_fast(TArg1 rcx = TArg1(), TArg2 rdx = TArg2(), TArg3 r8 = TArg3(), TArg4 r9 = TArg4()) noexcept
{
  detail::u64_t<TArg1>   a1{};
  detail::u64_t<TArg2>   a2{};
  detail::u64_t<TArg3>   a3{};
  detail::u64_t<TArg4>   a4{};
  detail::u64_t<TResult> r {};

  a1.as_value = rcx;
  a2.as_value = rdx;
  a3.as_value = r8;
  a4.as_value = r9;

  r.as_uint64_t = ia32_asm_vmx_vmcall(
    a1.as_uint64_t,
    a2.as_uint64_t,
    a3.as_uint64_t,
    a4.as_uint64_t);

  return r.as_value;
}

template <
  typename TResult = uint64_t,
  typename TArg1   = uint64_t,
  typename TArg2   = uint64_t,
  typename TArg3   = uint64_t,
  typename TArg4   = uint64_t,
  typename TArg5   = uint64_t,
  typename TArg6   = uint64_t,
  typename TArg7   = uint64_t,
  typename TArg8   = uint64_t,
  typename TArg9   = uint64_t,
  typename TArg10  = uint64_t
>
inline TResult vmcall_slow(TArg1 rcx = TArg1(), TArg2 rdx = TArg2(), TArg3 r8 = TArg3(), TArg4 r9 = TArg4(),
                           TArg5 r10 = TArg5(), TArg6 r11 = TArg6(), TArg7 r12 = TArg7(), TArg8 r13 = TArg8(),
                           TArg9 r14 = TArg9(), TArg10 r15 = TArg10()) noexcept
{
  detail::u64_t<TArg1>   a1{};
  detail::u64_t<TArg2>   a2{};
  detail::u64_t<TArg3>   a3{};
  detail::u64_t<TArg4>   a4{};
  detail::u64_t<TArg5>   a5{};
  detail::u64_t<TArg6>   a6{};
  detail::u64_t<TArg7>   a7{};
  detail::u64_t<TArg8>   a8{};
  detail::u64_t<TArg9>   a9{};
  detail::u64_t<TArg10>  a10{};
  detail::u64_t<TResult> r {};

  a1.as_value = rcx;
  a2.as_value = rdx;
  a3.as_value = r8;
  a4.as_value = r9;
  a5.as_value = r10;
  a6.as_value = r11;
  a7.as_value = r12;
  a8.as_value = r13;
  a9.as_value = r14;
  a10.as_value = r15;

  r.as_uint64_t = ia32_asm_vmx_vmcall_ex(
    a1.as_uint64_t,
    a2.as_uint64_t,
    a3.as_uint64_t,
    a4.as_uint64_t,
    a5.as_uint64_t,
    a6.as_uint64_t,
    a7.as_uint64_t,
    a8.as_uint64_t,
    a9.as_uint64_t,
    a10.as_uint64_t);

  return r.as_value;
}

template <
  typename TResult = uint64_t,
  typename ...TArgs
>
inline TResult vmcall(TArgs... args) noexcept
{
  if constexpr (sizeof...(args) <= 4)
  {
    return vmcall_fast<TResult, TArgs...>(args...);
  }
  else if constexpr (sizeof...(args) <= 10)
  {
    return vmcall_slow<TResult, TArgs...>(args...);
  }
  else
  {
    static_assert("too many arguments");
  }
}

inline error_code invept(invept_t type, invept_desc_t* descriptor = nullptr) noexcept
{
  if (!descriptor)
  {
    static invept_desc_t zero_descriptor{};
    descriptor = &zero_descriptor;
  }

  return static_cast<error_code>(ia32_asm_inv_ept(type, descriptor));
}

inline error_code invept_single_context(ept_ptr_t ept_pointer) noexcept
{
  invept_desc_t descriptor = { ept_pointer.flags, 0 };
  return invept(invept_t::single_context, &descriptor);
}

inline error_code invept_all_contexts() noexcept
{
  return invept(invept_t::all_contexts);
}

inline error_code invvpid(invvpid_t type, invvpid_desc_t* descriptor = nullptr) noexcept
{
  if (!descriptor)
  {
    static invvpid_desc_t zero_descriptor{};
    descriptor = &zero_descriptor;
  }

  return static_cast<error_code>(ia32_asm_inv_vpid(type, descriptor));
}

inline error_code invvpid_individual_address(uint16_t vpid, uint64_t linear_address) noexcept
{
  invvpid_desc_t descriptor = { vpid, 0, linear_address };
  return invvpid(invvpid_t::individual_address, &descriptor);
}

inline error_code invvpid_single_context(uint16_t vpid) noexcept
{
  invvpid_desc_t descriptor = { vpid, 0, 0 };
  return invvpid(invvpid_t::single_context, &descriptor);
}

inline error_code invvpid_all_contexts() noexcept
{
  return invvpid(invvpid_t::all_contexts);
}

inline error_code invvpid_single_context_retaining_globals(uint16_t vpid) noexcept
{
  invvpid_desc_t descriptor = { vpid, 0, 0 };
  return invvpid(invvpid_t::single_context_retaining_globals, &descriptor);
}

}
