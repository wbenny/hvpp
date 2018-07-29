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
};


struct pagefault_error_code
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

struct exception_error_code
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

    pagefault_error_code pagefault;
  };
};

}

