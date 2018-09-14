#pragma once
#include <cstdint>

namespace ia32::vmx {

struct exception_bitmap_t
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t divide_error                : 1;
      uint32_t debug                       : 1;
      uint32_t nmi_interrupt               : 1;
      uint32_t breakpoint                  : 1;
      uint32_t overflow                    : 1;
      uint32_t bound                       : 1;
      uint32_t invalid_opcode              : 1;
      uint32_t device_not_available        : 1;
      uint32_t double_fault                : 1;
      uint32_t coprocessor_segment_overrun : 1;
      uint32_t invalid_tss                 : 1;
      uint32_t segment_not_present         : 1;
      uint32_t stack_segment_fault         : 1;
      uint32_t general_protection          : 1;
      uint32_t page_fault                  : 1;
      uint32_t reserved                    : 1;
      uint32_t x87_floating_point_error    : 1;
      uint32_t alignment_check             : 1;
      uint32_t machine_check               : 1;
      uint32_t simd_floating_point_error   : 1;
      uint32_t virtualization_exception    : 1;
    };
  };
};

}
