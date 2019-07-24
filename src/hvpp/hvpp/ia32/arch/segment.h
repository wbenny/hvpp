#pragma once
#include "../msr.h"
#include "../msr/arch.h"

#include <cstdint>
#include <type_traits>

namespace ia32 {

//
// Segment selector
//

struct segment_selector_t
{
  enum
  {
    table_gdt = 0,
    table_ldt = 1,
  };

  union
  {
    uint16_t flags;

    struct
    {
      uint16_t request_privilege_level : 2;
      uint16_t table : 1;
      uint16_t index : 13;
    };
  };
};

struct cs_t   : segment_selector_t {};
struct ds_t   : segment_selector_t {};
struct es_t   : segment_selector_t {};
struct fs_t   : segment_selector_t {};
struct gs_t   : segment_selector_t {};
struct ss_t   : segment_selector_t {};
struct tr_t   : segment_selector_t {};
struct ldtr_t : segment_selector_t {};

//
// Segment access
//

struct segment_access_t
{
  //
  // System-Segment and Gate-Descriptor Types
  // (when descriptor_type == 0 (* descriptor_type_system *))
  //

  enum
  {
    type_reserved_0          = 0b0000,
    type_16b_tss_available   = 0b0001,
    type_ldt                 = 0b0010,
    type_16b_tss_busy        = 0b0011,
    type_16b_call_gate       = 0b0100,
    type_task_gate           = 0b0101,
    type_16b_interrupt_gate  = 0b0110,
    type_16b_trap_gate       = 0b0111,
    type_reserved_8          = 0b1000,
    type_32b_tss_available   = 0b1001,
    type_reserved_10         = 0b1010,
    type_32b_tss_busy        = 0b1011,
    type_32b_call_gate       = 0b1100,
    type_reserved_13         = 0b1101,
    type_32b_interrupt_gate  = 0b1110,
    type_32b_trap_gate       = 0b1111,

    //
    // Difference between:
    //   type_16b_tss_available / type_16b_tss_busy
    //   type_32b_tss_available / type_32b_tss_busy
    //

    type_tss_busy_flag       = 0b0010,
  };

  //
  // Code- and Data-Segment Types
  // (when descriptor_type == 1 (* descriptor_type_code_or_data *))
  //

  enum
  {
    type_read_only                        = 0b0000,
    type_read_only_accessed               = 0b0001,
    type_read_write                       = 0b0010,
    type_read_write_accessed              = 0b0011,
    type_read_only_expand_down            = 0b0100,
    type_read_only_expand_down_accessed   = 0b0101,
    type_read_write_expand_down           = 0b0110,
    type_read_write_expand_down_accessed  = 0b0111,
    type_execute_only                     = 0b1000,
    type_execute_only_accessed            = 0b1001,
    type_execute_read                     = 0b1010,
    type_execute_read_accessed            = 0b1011,
    type_execute_only_conforming          = 0b1100,
    type_execute_only_conforming_accessed = 0b1101,
    type_execute_read_conforming          = 0b1110,
    type_execute_read_conforming_accessed = 0b1111,
  };

  enum
  {
    descriptor_type_system = 0,
    descriptor_type_code_or_data = 1,
  };

  enum
  {
    granularity_byte = 0,
    granularity_4kb = 1,
  };

  union
  {
    uint16_t flags;

    struct
    {
      uint16_t type : 4;
      uint16_t descriptor_type : 1;
      uint16_t descriptor_privilege_level : 2;
      uint16_t present : 1;
      uint16_t limit_high : 4; // or reserved
      uint16_t available_bit : 1;
      uint16_t long_mode : 1;
      uint16_t default_big : 1;
      uint16_t granularity : 1;
    };

    struct
    {
      uint16_t type_accessed : 1;
      uint16_t type_write_enable : 1;
      uint16_t type_expansion_direction : 1;
      uint16_t type_code_segment : 1;
    };
  };
};

//
// Segment access (as represented in VMX)
//

struct segment_access_vmx_t
  : segment_access_t
{
  struct
  {
    uint16_t unusable; // : 1
  };
};

//
// IDT entry access
//

struct idt_access_t
{
  union
  {
    uint16_t flags;

    struct
    {
      uint16_t ist_index : 3;
      uint16_t reserved : 5;
      uint16_t type : 4;
      uint16_t descriptor_type : 1;
      uint16_t descriptor_privilege_level : 2;
      uint16_t present : 1;
    };
  };
};

static_assert(sizeof(segment_access_t)     == 2);
static_assert(sizeof(segment_access_vmx_t) == 4);

static_assert(sizeof(idt_access_t)         == 2);

//
// Descriptors
//

#pragma pack(push, 1)
struct gdt_entry_t
{
  uint16_t  limit_low;
  uint16_t  base_address_low;
  uint8_t   base_address_middle;
  segment_access_t access;
  uint8_t   base_address_high;
  uint32_t  base_address_upper;

  uint32_t  must_be_zero;

  void* base_address() const noexcept
  {
    uint64_t result = (
      (uint64_t(base_address_low))          |
      (uint64_t(base_address_middle) << 16) |
      (uint64_t(base_address_high)   << 24)
      );

    if (!access.descriptor_type)
    {
      result |= uint64_t(base_address_upper) << 32;
    }

    return reinterpret_cast<void*>(result);
  }

  void base_address(void* value) noexcept
  {
    base_address_low    = uint16_t(uint64_t(value) >>  0);
    base_address_middle =  uint8_t(uint64_t(value) >> 16);
    base_address_high   =  uint8_t(uint64_t(value) >> 24);
    base_address_upper  = uint32_t(uint64_t(value) >> 32);
  }

  uint32_t limit() const noexcept
  {
    return
      (uint32_t(       limit_low)) |
      (uint32_t(access.limit_high) << 16);
  }

  void limit(uint32_t value) noexcept
  {
            limit_low = uint16_t(uint64_t(value) >>  0);
    access.limit_high = uint16_t(uint64_t(value) >> 16);
  }

  //
  // Used for LDT.
  //

  const gdt_entry_t& at(segment_selector_t selector) const noexcept
  {
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address()) + selector.index * 8
      );
  }

  gdt_entry_t& at(segment_selector_t selector) noexcept
  {
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address()) + selector.index * 8
      );
  }

  const gdt_entry_t& operator[](segment_selector_t selector) const noexcept
  { return at(selector); }

  gdt_entry_t& operator[](segment_selector_t selector) noexcept
  { return at(selector); }
};

struct idt_entry_t
{
  uint16_t  base_address_low;
  segment_selector_t selector;
  idt_access_t access;
  uint16_t  base_address_middle;
  uint32_t  base_address_high;

  uint32_t  reserved;

  void* base_address() const noexcept
  {
    uint64_t result = (
      (uint64_t(base_address_low))          |
      (uint64_t(base_address_middle) << 16) |
      (uint64_t(base_address_high)   << 32)
      );

    return reinterpret_cast<void*>(result);
  }

  void base_address(void* value) noexcept
  {
    base_address_low    = uint16_t(uint64_t(value) >>  0);
    base_address_middle = uint16_t(uint64_t(value) >> 16);
    base_address_high   = uint32_t(uint64_t(value) >> 32);
  }
};

struct descriptor_table32_t
{
  uint16_t limit;
  uint32_t base_address;

  //
  // GDT entries are accessed by segment selector.
  //

  const gdt_entry_t& at(segment_selector_t selector) const noexcept
  {
    //
    // See explanation of (selector.index * 8) in vcpu.inl.
    //
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address) + selector.index * 8
      );
  }

  gdt_entry_t& at(segment_selector_t selector) noexcept
  {
    //
    // See explanation of (selector.index * 8) in vcpu.inl.
    //
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address) + selector.index * 8
      );
  }

  //
  // IDT entries are accessed by numeric index.
  //

  const idt_entry_t& at(int index) const noexcept
  {
    return reinterpret_cast<idt_entry_t*>(
      base_address
      )[index];
  }

  idt_entry_t& at(int index) noexcept
  {
    return reinterpret_cast<idt_entry_t*>(
      base_address
      )[index];
  }

  const gdt_entry_t& operator[](segment_selector_t selector) const noexcept
  { return at(selector); }

  gdt_entry_t& operator[](segment_selector_t selector) noexcept
  { return at(selector); }

  const idt_entry_t& operator[](int index) const noexcept
  { return at(index); }

  idt_entry_t& operator[](int index) noexcept
  { return at(index); }
};

struct descriptor_table64_t
{
  uint16_t limit;
  uint64_t base_address;

  const gdt_entry_t& at(segment_selector_t selector) const noexcept
  {
    //
    // See explanation of (selector.index * 8) in vcpu.inl.
    //
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address) + selector.index * 8
      );
  }

  gdt_entry_t& at(segment_selector_t selector) noexcept
  {
    //
    // See explanation of (selector.index * 8) in vcpu.inl.
    //
    return *reinterpret_cast<gdt_entry_t*>(
      uint64_t(base_address) + selector.index * 8
      );
  }

  const idt_entry_t& at(int index) const noexcept
  {
    return reinterpret_cast<idt_entry_t*>(
      base_address
      )[index];
  }

  idt_entry_t& at(int index) noexcept
  {
    return reinterpret_cast<idt_entry_t*>(
      base_address
      )[index];
  }

  const gdt_entry_t& operator[](segment_selector_t selector) const noexcept
  { return at(selector); }

  gdt_entry_t& operator[](segment_selector_t selector) noexcept
  { return at(selector); }

  const idt_entry_t& operator[](int index) const noexcept
  { return at(index); }

  idt_entry_t& operator[](int index) noexcept
  { return at(index); }
};
#pragma pack(pop)

struct gdtr32_t : descriptor_table32_t {};
struct idtr32_t : descriptor_table32_t {};

struct gdtr64_t : descriptor_table64_t {};
struct idtr64_t : descriptor_table64_t {};

using desc_table_t = descriptor_table64_t;
using gdtr_t = gdtr64_t;
using idtr_t = idtr64_t;

static_assert(sizeof(gdt_entry_t) == 16);
static_assert(sizeof(idt_entry_t) == 16);

static_assert(sizeof(descriptor_table32_t) == 6);
static_assert(sizeof(gdtr32_t) == 6);
static_assert(sizeof(idtr32_t) == 6);

static_assert(sizeof(descriptor_table64_t) == 10);
static_assert(sizeof(gdtr64_t) == 10);
static_assert(sizeof(idtr64_t) == 10);

//
// Forward declaration (from ia32/arch.h)
//
template <typename T> T read() noexcept;

//
// Segment
//

template <typename T = segment_selector_t>
struct segment_t
{
  static_assert(std::is_base_of_v<segment_selector_t, T>);

  void*     base_address;
  uint32_t  limit;
  segment_access_vmx_t access;
  T         selector;

  segment_t() noexcept
    : base_address{}
    , limit{}
    , access{}
    , selector{}
  { }

  segment_t(T selector) noexcept
    : base_address{}
    , limit{}
    , access{}
    , selector{ selector }
  { }

  segment_t(T selector, void* base_address) noexcept
    : base_address{ base_address }
    , limit{}
    , access{}
    , selector{ selector }
  { }

  segment_t(void* base_address, uint32_t limit, segment_access_vmx_t access, T selector) noexcept
    : base_address{ base_address }
    , limit{ limit }
    , access{ access }
    , selector{ selector }
  { }

  segment_t(descriptor_table64_t descriptor_table, T segment_selector) noexcept
  {
    static_assert(sizeof(segment_t) == 24);

    if (!segment_selector.flags)
    {
      selector.flags = 0;
      access.flags = 0;
      access.unusable = true;
      limit = 0;
      base_address = nullptr;
    }
    else
    {
      selector = segment_selector;
      access.flags = static_cast<uint16_t>(ia32_asm_read_ar(selector.flags) >> 8);
      access.unusable = !selector.flags;
      access.limit_high = 0;
      limit = ia32_asm_read_sl(selector.flags);

      if constexpr (std::is_same_v<T, fs_t>)
      {
        (void)(descriptor_table);
        base_address = reinterpret_cast<void*>(msr::read<msr::fs_base_t>());
      }
      else if constexpr (std::is_same_v<T, gs_t>)
      {
        (void)(descriptor_table);
        base_address = reinterpret_cast<void*>(msr::read<msr::gs_base_t>());
      }
      else
      {
        //
        // Actually untested with LDT.
        //
        if (selector.table == segment_selector_t::table_ldt) ia32_asm_int3();

        const auto& table_entry = selector.table
          ? descriptor_table[read<ldtr_t>()][selector]
          : descriptor_table[                selector];

        base_address = table_entry.base_address();
      }
    }
  }
};

}
