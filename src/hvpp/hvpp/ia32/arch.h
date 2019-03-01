#pragma once
#include "asm.h"

#include "arch/cr.h"
#include "arch/dr.h"
#include "arch/rflags.h"
#include "arch/segment.h"
#include "arch/xsave.h"

#include <cstdint>
#include <cstring> // memset

namespace ia32 {

struct context_t
{
  enum
  {
    reg_rax   =  0,
    reg_rcx   =  1,
    reg_rdx   =  2,
    reg_rbx   =  3,
    reg_rsp   =  4,
    reg_rbp   =  5,
    reg_rsi   =  6,
    reg_rdi   =  7,
    reg_r8    =  8,
    reg_r9    =  9,
    reg_r10   = 10,
    reg_r11   = 11,
    reg_r12   = 12,
    reg_r13   = 13,
    reg_r14   = 14,
    reg_r15   = 15,

    reg_min   =  0,
    reg_max   = 15,
  };

  enum
  {
    seg_es    = 0,
    seg_cs    = 1,
    seg_ss    = 2,
    seg_ds    = 3,
    seg_fs    = 4,
    seg_gs    = 5,
    seg_ldtr  = 6,
    seg_tr    = 7,

    seg_min   = 0,
    seg_max   = 7,
  };

  union
  {
    struct
    {
      union
      {
        uint64_t rax;
        void*    rax_as_pointer;

        struct
        {
          uint32_t eax;
          uint32_t rax_hi;
        };
      };

      union
      {
        uint64_t rcx;
        void*    rcx_as_pointer;

        struct
        {
          uint32_t ecx;
          uint32_t rcx_hi;
        };
      };

      union
      {
        uint64_t rdx;
        void*    rdx_as_pointer;

        struct
        {
          uint32_t edx;
          uint32_t rdx_hi;
        };
      };

      union
      {
        uint64_t rbx;
        void*    rbx_as_pointer;

        struct
        {
          uint32_t ebx;
          uint32_t rbx_hi;
        };
      };

      union
      {
        uint64_t rsp;
        void*    rsp_as_pointer;

        struct
        {
          uint32_t esp;
          uint32_t rsp_hi;
        };
      };

      union
      {
        uint64_t rbp;
        void*    rbp_as_pointer;

        struct
        {
          uint32_t ebp;
          uint32_t rbp_hi;
        };
      };

      union
      {
        uint64_t rsi;
        void*    rsi_as_pointer;

        struct
        {
          uint32_t esi;
          uint32_t rsi_hi;
        };
      };

      union
      {
        uint64_t rdi;
        void*    rdi_as_pointer;

        struct
        {
          uint32_t edi;
          uint32_t rdi_hi;
        };
      };

      union
      {
        uint64_t r8;
        void*    r8_as_pointer;

        struct
        {
          uint32_t r8d;
          uint32_t r8_hi;
        };
      };

      union
      {
        uint64_t r9;
        void*    r9_as_pointer;

        struct
        {
          uint32_t r9d;
          uint32_t r9_hi;
        };
      };

      union
      {
        uint64_t r10;
        void*    r10_as_pointer;

        struct
        {
          uint32_t r10d;
          uint32_t r10_hi;
        };
      };

      union
      {
        uint64_t r11;
        void*    r11_as_pointer;

        struct
        {
          uint32_t r11d;
          uint32_t r11_hi;
        };
      };

      union
      {
        uint64_t r12;
        void*    r12_as_pointer;

        struct
        {
          uint32_t r12d;
          uint32_t r12_hi;
        };
      };

      union
      {
        uint64_t r13;
        void*    r13_as_pointer;

        struct
        {
          uint32_t r13d;
          uint32_t r13_hi;
        };
      };

      union
      {
        uint64_t r14;
        void*    r14_as_pointer;

        struct
        {
          uint32_t r14d;
          uint32_t r14_hi;
        };
      };

      union
      {
        uint64_t r15;
        void*    r15_as_pointer;

        struct
        {
          uint32_t r15d;
          uint32_t r15_hi;
        };
      };
    };

    uint64_t gp_register[16];
  };

  union
  {
    uint64_t rip;
    void*    rip_as_pointer;

    struct
    {
      uint32_t eip;
      uint32_t rip_hi;
    };
  };

  rflags_t rflags;

  int capture() noexcept;

  [[noreturn]]
  void restore() noexcept;

  void clear() noexcept
  { memset(this, 0, sizeof(*this)); }
};

inline bool operator==(const context_t& lhs, const context_t& rhs) noexcept
{
  return lhs.rip          == rhs.rip          &&
         lhs.rflags.flags == rhs.rflags.flags &&
         !memcmp(lhs.gp_register,
                 rhs.gp_register,
                 sizeof(lhs.gp_register));
}

inline bool operator!=(const context_t& lhs, const context_t& rhs) noexcept
{
  return !(lhs == rhs);
}

static_assert(sizeof(context_t) == 144);

template <typename T> T    read()         noexcept { ia32_asm_int3(); }
template <typename T> void write(T value) noexcept { ia32_asm_int3(); }

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
template <> inline dr0_t    read() noexcept { return dr0_t    { ia32_asm_read_dr0() };    }
template <> inline dr1_t    read() noexcept { return dr1_t    { ia32_asm_read_dr1() };    }
template <> inline dr2_t    read() noexcept { return dr2_t    { ia32_asm_read_dr2() };    }
template <> inline dr3_t    read() noexcept { return dr3_t    { ia32_asm_read_dr3() };    }
template <> inline dr4_t    read() noexcept { return dr4_t    { ia32_asm_read_dr4() };    }
template <> inline dr5_t    read() noexcept { return dr5_t    { ia32_asm_read_dr5() };    }
template <> inline dr6_t    read() noexcept { return dr6_t    { ia32_asm_read_dr6() };    }
template <> inline dr7_t    read() noexcept { return dr7_t    { ia32_asm_read_dr7() };    }

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
template <> inline void write(dr0_t value)    noexcept { ia32_asm_write_dr0(value.flags); }
template <> inline void write(dr1_t value)    noexcept { ia32_asm_write_dr1(value.flags); }
template <> inline void write(dr2_t value)    noexcept { ia32_asm_write_dr2(value.flags); }
template <> inline void write(dr3_t value)    noexcept { ia32_asm_write_dr3(value.flags); }
template <> inline void write(dr4_t value)    noexcept { ia32_asm_write_dr4(value.flags); }
template <> inline void write(dr5_t value)    noexcept { ia32_asm_write_dr5(value.flags); }
template <> inline void write(dr6_t value)    noexcept { ia32_asm_write_dr6(value.flags); }
template <> inline void write(dr7_t value)    noexcept { ia32_asm_write_dr7(value.flags); }

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
