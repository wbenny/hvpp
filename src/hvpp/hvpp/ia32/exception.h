#pragma once
#include <cstdint>

namespace ia32 {

enum class exception_vector : uint32_t
{
  divide_error                = 0,
  debug                       = 1,
  nmi_interrupt               = 2,
  breakpoint                  = 3,
  overflow                    = 4,
  bound                       = 5,
  invalid_opcode              = 6,
  device_not_available        = 7,
  double_fault                = 8,
  coprocessor_segment_overrun = 9,
  invalid_tss                 = 10,
  segment_not_present         = 11,
  stack_segment_fault         = 12,
  general_protection          = 13,
  page_fault                  = 14,
  x87_floating_point_error    = 16,
  alignment_check             = 17,
  machine_check               = 18,
  simd_floating_point_error   = 19,
  virtualization_exception    = 20,

  //
  // NT (Windows) specific exception vectors.
  //

  nt_apc_interrupt            = 31,
  nt_dpc_interrupt            = 47,
  nt_clock_interrupt          = 209,
  nt_pmi_interrupt            = 254,
};

struct pagefault_error_code_t
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t present : 1;
      uint32_t write : 1;
      uint32_t user_mode_access : 1;
      uint32_t reserved_bit_violation : 1;
      uint32_t execute : 1;
      uint32_t protection_key_violation : 1;
      uint32_t reserved_1 : 9;
      uint32_t sgx_access_violation : 1;
      uint32_t reserved_2 : 16;
    };
  };
};

struct exception_error_code_t
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t external_event : 1;
      uint32_t descriptor_location : 1;
      uint32_t table : 1;
      uint32_t index : 13;
      uint32_t reserved : 16;
    };

    pagefault_error_code_t pagefault;
  };
};

constexpr inline const char* to_string(exception_vector value) noexcept
{
  switch (value)
  {
    case exception_vector::divide_error: return "divide_error";
    case exception_vector::debug: return "debug";
    case exception_vector::nmi_interrupt: return "nmi_interrupt";
    case exception_vector::breakpoint: return "breakpoint";
    case exception_vector::overflow: return "overflow";
    case exception_vector::bound: return "bound";
    case exception_vector::invalid_opcode: return "invalid_opcode";
    case exception_vector::device_not_available: return "device_not_available";
    case exception_vector::double_fault: return "double_fault";
    case exception_vector::coprocessor_segment_overrun: return "coprocessor_segment_overrun";
    case exception_vector::invalid_tss: return "invalid_tss";
    case exception_vector::segment_not_present: return "segment_not_present";
    case exception_vector::stack_segment_fault: return "stack_segment_fault";
    case exception_vector::general_protection: return "general_protection";
    case exception_vector::page_fault: return "page_fault";
    case exception_vector::x87_floating_point_error: return "x87_floating_point_error";
    case exception_vector::alignment_check: return "alignment_check";
    case exception_vector::machine_check: return "machine_check";
    case exception_vector::simd_floating_point_error: return "simd_floating_point_error";
    case exception_vector::virtualization_exception: return "virtualization_exception";
    case exception_vector::nt_apc_interrupt: return "nt_apc_interrupt";
    case exception_vector::nt_dpc_interrupt: return "nt_dpc_interrupt";
    case exception_vector::nt_clock_interrupt: return "nt_clock_interrupt";
    case exception_vector::nt_pmi_interrupt: return "nt_pmi_interrupt";
  }

  return "";
}

}

