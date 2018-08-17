#pragma once
#include <cstdint>
#include <type_traits>

namespace ia32 {

struct seg_access_t
{
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
  };
};

struct seg_access_vmx_t : seg_access_t
{
  struct
  {
    uint16_t unusable; // : 1
  };
};

static_assert(sizeof(seg_access_t)     == 2);
static_assert(sizeof(seg_access_vmx_t) == 4);

struct seg_selector_t
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
      uint16_t request_privilege_level                                 : 2;
      uint16_t table                                                   : 1;
      uint16_t index                                                   : 13;
    };
  };
};

struct cs_t : seg_selector_t {};
struct ds_t : seg_selector_t {};
struct es_t : seg_selector_t {};
struct fs_t : seg_selector_t {};
struct gs_t : seg_selector_t {};
struct ss_t : seg_selector_t {};
struct tr_t : seg_selector_t {};
struct ldtr_t : seg_selector_t {};

//
// Descriptors
//

struct seg_descriptor_entry_t;

#pragma pack(push, 1)
struct seg_descriptor_table_t
{
  uint16_t limit;
  void*    base_address;

  seg_descriptor_entry_t& at(seg_selector_t selector) noexcept
  {
    //
    // See explanation of (selector.index * 8) in vcpu.inl.
    //
    return *reinterpret_cast<seg_descriptor_entry_t*>
           (reinterpret_cast<uint64_t>
           (base_address) + selector.index * 8);
  }

  seg_descriptor_entry_t& operator[](seg_selector_t selector) noexcept
  {
    return at(selector);
  }
};
#pragma pack(pop)

struct gdtr_t : seg_descriptor_table_t {};
struct idtr_t : seg_descriptor_table_t {};

static_assert(sizeof(seg_descriptor_table_t) == 10);
static_assert(sizeof(gdtr_t) == 10);
static_assert(sizeof(idtr_t) == 10);

#pragma pack(push, 1)
struct seg_descriptor_entry_t
{
  uint16_t     limit_low;
  uint16_t     base_address_low;
  uint8_t      base_address_middle;
  seg_access_t access;
  uint8_t      base_address_high;
  uint32_t     base_address_upper;

  uint32_t     must_be_zero;

  void* base_address() const noexcept
  {
    uint64_t result = (
      (static_cast<uint64_t>(base_address_low))          |
      (static_cast<uint64_t>(base_address_middle) << 16) |
      (static_cast<uint64_t>(base_address_high)   << 24)
      );

    if (!access.descriptor_type)
    {
      result |= static_cast<uint64_t>(base_address_upper) << 32;
    }

    return reinterpret_cast<void*>(result);
  }

  uint32_t limit() const noexcept
  {
    return
      (static_cast<uint32_t>(       limit_low)) |
      (static_cast<uint32_t>(access.limit_high) << 16);
  }

  //
  // Used for LDT.
  //

  seg_descriptor_entry_t& at(seg_selector_t selector) noexcept
  {
    return *reinterpret_cast<seg_descriptor_entry_t*>
           (reinterpret_cast<uint64_t>
           (base_address()) + selector.index * 8);
  }

  seg_descriptor_entry_t& operator[](seg_selector_t selector) noexcept
  {
    return at(selector);
  }
};
#pragma pack(pop)

static_assert(sizeof(seg_descriptor_entry_t) == 16);

//
// Forward declaration (from ia32/arch.h)
//
template <typename T> T read() noexcept;

template <typename T = seg_selector_t>
struct seg_t
{
  static_assert(std::is_base_of_v<seg_selector_t, T>);

  void*             base_address;
  uint32_t          limit;
  seg_access_vmx_t  access;
  T                 selector;

  seg_t() noexcept
    : base_address()
    , limit()
    , access()
    , selector()
  {

  }

  seg_t(T selector) noexcept
    : base_address()
    , limit()
    , access()
    , selector(selector)
  {

  }

  seg_t(T selector, void* base_address) noexcept
    : base_address(base_address)
    , limit()
    , access()
    , selector(selector)
  {

  }

  seg_t(void* base_address, uint32_t limit, seg_access_vmx_t access, T selector) noexcept
    : base_address(base_address)
    , limit(limit)
    , access(access)
    , selector(selector)
  {

  }

  seg_t(seg_descriptor_table_t& descriptor_table, const T& segment_selector) noexcept
  {
    static_assert(sizeof(seg_t) == 24);

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
        if (selector.table == seg_selector_t::table_ldt) __debugbreak();

        auto& table_entry = selector.table
          ? descriptor_table[read<ldtr_t>()][selector]
          : descriptor_table[                selector];

        base_address = table_entry.base_address();
      }
    }
  }
};

}
