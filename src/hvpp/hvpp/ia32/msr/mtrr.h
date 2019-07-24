#pragma once
#include <cstdint>

namespace ia32::msr {

struct mtrr_capabilities_t
{
  static constexpr uint32_t msr_id = 0x000000FE;
  using result_type = mtrr_capabilities_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t variable_range_count : 8;
      uint64_t fixed_range_supported : 1;
      uint64_t reserved1 : 1;
      uint64_t wc_supported : 1;
      uint64_t smrr_supported : 1;
      uint64_t reserved2 : 52;
    };
  };
};

struct mtrr_def_type_t
{
  static constexpr uint32_t msr_id = 0x000002FF;
  using result_type = mtrr_def_type_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t default_memory_type : 8;
      uint64_t reserved1 : 2;
      uint64_t fixed_range_mtrr_enable : 1;
      uint64_t mtrr_enable : 1;
      uint64_t reserved2 : 52;
    };
  };
};

struct mtrr_physbase_t
{
  static constexpr uint32_t msr_id = 0x00000200; // IA32_MTRR_PHYSBASE0
  using result_type = mtrr_physbase_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t type : 8;
      uint64_t reserved1 : 4;
      uint64_t page_frame_number : 36;
      uint64_t reserved2 : 16;
    };
  };
};

struct mtrr_physmask_t
{
  static constexpr uint32_t msr_id = 0x00000201; // IA32_MTRR_PHYSMASK0
  using result_type = mtrr_physmask_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t type : 8;
      uint64_t reserved1 : 3;
      uint64_t valid : 1;
      uint64_t page_frame_number : 36;
      uint64_t reserved2 : 16;
    };
  };
};

template <
  uint32_t MSR_ID,
  uint64_t MTRR_BASE,
  uint64_t MTRR_SIZE
>
struct mtrr_fix_t
{
  static constexpr uint32_t msr_id    = MSR_ID;
  static constexpr uint64_t mtrr_base = MTRR_BASE;
  static constexpr uint64_t mtrr_size = MTRR_SIZE;
  using result_type = mtrr_fix_t<MSR_ID, MTRR_BASE, MTRR_SIZE>;

  union
  {
    uint64_t flags;

    struct
    {
      uint8_t type[8];
    };
  };
};

using mtrr_fix_64k_00000_t = mtrr_fix_t<0x0250, 0x00000, 0x10000>;
using mtrr_fix_16k_80000_t = mtrr_fix_t<0x0258, 0x80000,  0x4000>;
using mtrr_fix_16k_a0000_t = mtrr_fix_t<0x0259, 0xa0000,  0x4000>;
using mtrr_fix_4k_c0000_t  = mtrr_fix_t<0x0268, 0xc0000,  0x1000>;
using mtrr_fix_4k_c8000_t  = mtrr_fix_t<0x0269, 0xc8000,  0x1000>;
using mtrr_fix_4k_d0000_t  = mtrr_fix_t<0x026a, 0xd0000,  0x1000>;
using mtrr_fix_4k_d8000_t  = mtrr_fix_t<0x026b, 0xd8000,  0x1000>;
using mtrr_fix_4k_e0000_t  = mtrr_fix_t<0x026c, 0xe0000,  0x1000>;
using mtrr_fix_4k_e8000_t  = mtrr_fix_t<0x026d, 0xe8000,  0x1000>;
using mtrr_fix_4k_f0000_t  = mtrr_fix_t<0x026e, 0xf0000,  0x1000>;
using mtrr_fix_4k_f8000_t  = mtrr_fix_t<0x026f, 0xf8000,  0x1000>;

}
