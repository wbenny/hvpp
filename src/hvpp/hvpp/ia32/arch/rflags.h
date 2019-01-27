#pragma once
#include <cstdint>

namespace ia32 {

struct rflags_t
{
  //
  // Reserved bits.
  //
  static constexpr uint64_t reserved_bits = 0xffc38028;

  //
  // Bits that are fixed to 1 ("read_as_1" field).
  //
  static constexpr uint64_t fixed_bits    = 0x00000002;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t carry_flag : 1;
      uint64_t read_as_1 : 1;
      uint64_t parity_flag : 1;
      uint64_t reserved_1 : 1;
      uint64_t auxiliary_carry_flag : 1;
      uint64_t reserved_2 : 1;
      uint64_t zero_flag : 1;
      uint64_t sign_flag : 1;
      uint64_t trap_flag : 1;
      uint64_t interrupt_enable_flag : 1;
      uint64_t direction_flag : 1;
      uint64_t overflow_flag : 1;
      uint64_t io_privilege_level : 2;
      uint64_t nested_task_flag : 1;
      uint64_t reserved_3 : 1;
      uint64_t resume_flag : 1;
      uint64_t virtual_8086_mode_flag : 1;
      uint64_t alignment_check_flag : 1;
      uint64_t virtual_interrupt_flag : 1;
      uint64_t virtual_interrupt_pending_flag : 1;
      uint64_t identification_flag : 1;
    };
  };
};

}
