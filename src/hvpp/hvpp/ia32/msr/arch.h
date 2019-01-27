#pragma once
#include <cstdint>

namespace ia32::msr {

struct apic_base_t
{
  static constexpr uint32_t msr_id = 0x0000001b;
  using result_type = apic_base_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 8;
      uint64_t bsp_flag : 1;
      uint64_t reserved_2 : 1;
      uint64_t enable_x2apic_mode : 1;
      uint64_t apic_global_enable : 1;
      uint64_t page_frame_number : 36;
    };
  };
};

struct debugctl_t
{
  static constexpr uint32_t msr_id = 0x000001d9;
  using result_type = debugctl_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t lbr : 1;
      uint64_t btf : 1;
      uint64_t reserved_1 : 4;
      uint64_t tr : 1;
      uint64_t bts : 1;
      uint64_t btint : 1;
      uint64_t bts_off_os : 1;
      uint64_t bts_off_usr : 1;
      uint64_t freeze_lbrs_on_pmi : 1;
      uint64_t freeze_perfmon_on_pmi : 1;
      uint64_t enable_uncore_pmi : 1;
      uint64_t freeze_while_smm : 1;
      uint64_t rtm_debug : 1;
    };
  };
};

struct efer_t
{
  static constexpr uint32_t msr_id = 0xc0000080;
  using result_type = efer_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t syscall_enable : 1;
      uint64_t reserved1 : 7;
      uint64_t ia32e_mode_enable : 1;
      uint64_t reserved2 : 1;
      uint64_t ia32e_mode_active : 1;
      uint64_t execute_disable_bit_enable : 1;
      uint64_t reserved3 : 52;
    };
  };
};

struct star_t
{
  static constexpr uint32_t msr_id = 0xc0000081;
  using result_type = uint64_t;
};

struct lstar_t
{
  static constexpr uint32_t msr_id = 0xc0000082;
  using result_type = uint64_t;
};

struct cstar_t
{
  static constexpr uint32_t msr_id = 0xc0000083;
  using result_type = uint64_t;
};

struct fmask_t
{
  static constexpr uint32_t msr_id = 0xc0000084;
  using result_type = rflags_t;
};

struct fs_base_t
{
  static constexpr uint32_t msr_id = 0xc0000100;
  using result_type = uint64_t;
};

struct gs_base_t
{
  static constexpr uint32_t msr_id = 0xc0000101;
  using result_type = uint64_t;
};

}
