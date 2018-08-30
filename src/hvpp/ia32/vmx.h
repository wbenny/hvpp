#pragma once
#include "asm.h"
#include "ept.h"
#include "msr.h"
#include "memory.h"

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
  static_assert(sizeof(T) <= 8);

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
    std::is_same_v<T, cr0_t>                   ||
    std::is_same_v<T, cr4_t>;

  constexpr bool is_dr6 =
    std::is_same_v<T, dr6_t>;

  constexpr bool is_dr7 =
    std::is_same_v<T, dr7_t>;

  constexpr bool is_vmx_ctl_msr =
    std::is_same_v<T, msr::vmx_pinbased_ctls_t>  ||
    std::is_same_v<T, msr::vmx_procbased_ctls_t> ||
    std::is_same_v<T, msr::vmx_entry_ctls_t>     ||
    std::is_same_v<T, msr::vmx_exit_ctls_t>;

  constexpr bool is_vmx_procbased_ctls2_msr =
    std::is_same_v<T, msr::vmx_procbased_ctls2_t>;

  static_assert(is_control_register || is_dr6|| is_dr7 || is_vmx_ctl_msr ||
                is_vmx_procbased_ctls2_msr,
                "type is not adjustable");

  if constexpr (is_control_register)
  {
    using fixed0_msr_t = std::conditional_t<std::is_same_v<T, cr0_t>, msr::vmx_cr0_fixed0_t, msr::vmx_cr4_fixed0_t>;
    using fixed1_msr_t = std::conditional_t<std::is_same_v<T, cr0_t>, msr::vmx_cr0_fixed1_t, msr::vmx_cr4_fixed1_t>;

    auto cr_fixed0 = msr::read<fixed0_msr_t>();
    auto cr_fixed1 = msr::read<fixed1_msr_t>();

    desired.flags |= cr_fixed0.flags;
    desired.flags &= cr_fixed1.flags;
  }
  else if constexpr (is_dr6)
  {
    //
    // x |= ~x will set all bits of the bitfield to 1's. It would be prettier to
    // just use ~0, but compilers would complain about value not fitting into
    // bitfield.
    //
    // Vol3B[17.2(Debug Registers)] describes which reserved fields should be
    // set to 0 and 1 respectively.
    //
    desired.reserved_1 |= ~desired.reserved_1;
    desired.reserved_2 = 0;
    desired.reserved_3 |= ~desired.reserved_3;
  }
  else if constexpr (is_dr7)
  {
    desired.reserved_1 |= ~desired.reserved_1;
    desired.reserved_2 = 0;
    desired.reserved_3 = 0;
  }
  else if constexpr (is_vmx_ctl_msr)
  {
    constexpr uint32_t true_msr_id = T::msr_id + 0xC;
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
  typename TArg1 = uint64_t,
  typename TArg2 = uint64_t,
  typename TArg3 = uint64_t,
  typename TArg4 = uint64_t
>
inline TResult vmcall(TArg1 rcx = TArg1(), TArg2 rdx = TArg2(), TArg3 r8 = TArg3(), TArg4 r9 = TArg4()) noexcept
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

enum class invept_t : uint32_t
{
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
};

enum class invvpid_t : uint32_t
{
  individual_address                = 0x00000000,
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
  single_context_retaining_globals  = 0x00000003,
};

struct invept_desc_t
{
  ept_ptr_t ept_pointer;
  uint64_t  reserved;
};

static_assert(sizeof(invept_desc_t) == 16);

struct invvpid_desc_t
{
  uint64_t vpid : 16;
  uint64_t reserved : 48;
  uint64_t linear_address;
};

static_assert(sizeof(invvpid_desc_t) == 16);

inline error_code invept(invept_t type, invept_desc_t* descriptor = nullptr) noexcept
{
  if (!descriptor)
  {
    static invept_desc_t zero_descriptor{};
    descriptor = &zero_descriptor;
  }

  return static_cast<error_code>(ia32_asm_inv_ept(static_cast<uint32_t>(type), descriptor));
}

inline error_code invept_single_context(ept_ptr_t ept_pointer) noexcept
{
  invept_desc_t descriptor = { ept_pointer, 0 };
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

  return static_cast<error_code>(ia32_asm_inv_vpid(static_cast<uint32_t>(type), descriptor));
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
