#pragma once
#include "arch/cr.h"
#include "arch/dr.h"
#include "arch/rflags.h"
#include "arch/segment.h"

#include <cstdint>

namespace ia32 {

struct context
{
  enum
  {
    reg_rax =  0,
    reg_rcx =  1,
    reg_rdx =  2,
    reg_rbx =  3,
    reg_rsp =  4,
    reg_rbp =  5,
    reg_rsi =  6,
    reg_rdi =  7,
    reg_r8  =  8,
    reg_r9  =  9,
    reg_r10 = 10,
    reg_r11 = 11,
    reg_r12 = 12,
    reg_r13 = 13,
    reg_r14 = 14,
    reg_r15 = 15,
  };

  union
  {
    struct
    {
      uint64_t rax;
      uint64_t rcx;
      uint64_t rdx;
      uint64_t rbx;
      uint64_t rsp;
      uint64_t rbp;
      uint64_t rsi;
      uint64_t rdi;
      uint64_t r8;
      uint64_t r9;
      uint64_t r10;
      uint64_t r11;
      uint64_t r12;
      uint64_t r13;
      uint64_t r14;
      uint64_t r15;
    };

    uint64_t gp_register[16];
  };

  uint64_t rip;
  rflags_t rflags;

  int  capture() noexcept;

  [[noreturn]]
  void restore() noexcept;
};

static_assert(sizeof(context) == 144);

template <typename T> T    read()         noexcept { static_assert(0, "invalid specialization"); }
template <typename T> void write(T value) noexcept { static_assert(0, "invalid specialization"); }

//
// ====================
//   Read operations
// ====================
//

//
// Control registers
//
template <> inline cr0_t    read() noexcept { return cr0_t    { ia32_asm_read_cr0() };    }
template <> inline cr2_t    read() noexcept { return cr2_t    { ia32_asm_read_cr2() };    }
template <> inline cr3_t    read() noexcept { return cr3_t    { ia32_asm_read_cr3() };    }
template <> inline cr4_t    read() noexcept { return cr4_t    { ia32_asm_read_cr4() };    }

//
// Debug registers
//
template <> inline dr0_t    read() noexcept { return dr0_t    { ia32_asm_read_dr(0) };    }
template <> inline dr1_t    read() noexcept { return dr1_t    { ia32_asm_read_dr(1) };    }
template <> inline dr2_t    read() noexcept { return dr2_t    { ia32_asm_read_dr(2) };    }
template <> inline dr3_t    read() noexcept { return dr3_t    { ia32_asm_read_dr(3) };    }
template <> inline dr4_t    read() noexcept { return dr4_t    { ia32_asm_read_dr(4) };    }
template <> inline dr5_t    read() noexcept { return dr5_t    { ia32_asm_read_dr(5) };    }
template <> inline dr6_t    read() noexcept { return dr6_t    { ia32_asm_read_dr(6) };    }
template <> inline dr7_t    read() noexcept { return dr7_t    { ia32_asm_read_dr(7) };    }

//
// RFLAGS
//
template <> inline rflags_t read() noexcept { return rflags_t { ia32_asm_read_eflags() }; }

//
// Selectors
//
template <> inline cs_t     read() noexcept { return cs_t     { ia32_asm_read_cs() };     }
template <> inline ds_t     read() noexcept { return ds_t     { ia32_asm_read_ds() };     }
template <> inline es_t     read() noexcept { return es_t     { ia32_asm_read_es() };     }
template <> inline fs_t     read() noexcept { return fs_t     { ia32_asm_read_fs() };     }
template <> inline ss_t     read() noexcept { return ss_t     { ia32_asm_read_ss() };     }
template <> inline gs_t     read() noexcept { return gs_t     { ia32_asm_read_gs() };     }
template <> inline tr_t     read() noexcept { return tr_t     { ia32_asm_read_tr() };     }
template <> inline ldtr_t   read() noexcept { return ldtr_t   { ia32_asm_read_ldtr() };   }

//
// Descriptors
//
template <> inline gdtr_t   read() noexcept { gdtr_t result; ia32_asm_read_gdtr(&result); return result; }
template <> inline idtr_t   read() noexcept { idtr_t result; ia32_asm_read_idtr(&result); return result; }


//
// ====================
//   Write operations
// ====================
//

//
// Control registers
//
template <> inline void write(cr0_t value)    noexcept { ia32_asm_write_cr0(value.flags); }
template <> inline void write(cr2_t value)    noexcept { ia32_asm_write_cr2(value.flags); }
template <> inline void write(cr3_t value)    noexcept { ia32_asm_write_cr3(value.flags); }
template <> inline void write(cr4_t value)    noexcept { ia32_asm_write_cr4(value.flags); }

//
// Debug registers
//
template <> inline void write(dr0_t value)    noexcept { ia32_asm_write_dr(0, value.flags); }
template <> inline void write(dr1_t value)    noexcept { ia32_asm_write_dr(1, value.flags); }
template <> inline void write(dr2_t value)    noexcept { ia32_asm_write_dr(2, value.flags); }
template <> inline void write(dr3_t value)    noexcept { ia32_asm_write_dr(3, value.flags); }
template <> inline void write(dr4_t value)    noexcept { ia32_asm_write_dr(4, value.flags); }
template <> inline void write(dr5_t value)    noexcept { ia32_asm_write_dr(5, value.flags); }
template <> inline void write(dr6_t value)    noexcept { ia32_asm_write_dr(6, value.flags); }
template <> inline void write(dr7_t value)    noexcept { ia32_asm_write_dr(7, value.flags); }

//
// RFLAGS
//
template <> inline void write(rflags_t value) noexcept { ia32_asm_write_eflags(value.flags); }

//
// Selectors
//
template <> inline void write(cs_t value)     noexcept { ia32_asm_write_cs(value.flags); }
template <> inline void write(ds_t value)     noexcept { ia32_asm_write_ds(value.flags); }
template <> inline void write(es_t value)     noexcept { ia32_asm_write_es(value.flags); }
template <> inline void write(fs_t value)     noexcept { ia32_asm_write_fs(value.flags); }
template <> inline void write(gs_t value)     noexcept { ia32_asm_write_gs(value.flags); }
template <> inline void write(ss_t value)     noexcept { ia32_asm_write_ss(value.flags); }
template <> inline void write(tr_t value)     noexcept { ia32_asm_write_tr(value.flags); }
template <> inline void write(ldtr_t value)   noexcept { ia32_asm_write_ldtr(value.flags); }

//
// Descriptors
//
template <> inline void write(gdtr_t value)   noexcept { ia32_asm_write_gdtr(&value); }
template <> inline void write(idtr_t value)   noexcept { ia32_asm_write_idtr(&value); }

}
