#pragma once
#include "memory.h"

#include <cstdint>

namespace ia32 {

//
// EPT Page Table Entry
//
struct ept_pte_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t memory_type : 3;
      uint64_t ignore_pat : 1;
      uint64_t reserved_1 : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_2 : 1;
      uint64_t page_frame_number : 36;
      uint64_t reserved_3 : 15;
      uint64_t suppress_ve : 1;
    };
  };
};

//
// EPT Page Directory Entry
//
struct ept_pde_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t reserved_1 : 5;
      uint64_t accessed : 1;
      uint64_t reserved_2 : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_3 : 1;
      uint64_t page_frame_number : 36;
    };
  };
};

//
// EPT Large Page Directory Entry
//
struct ept_pde_large_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t memory_type : 3;
      uint64_t ignore_pat : 1;
      uint64_t large_page : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_1 : 10;
      uint64_t page_frame_number : 27;
      uint64_t reserved_2 : 15;
      uint64_t suppress_ve : 1;
    };
  };
};

//
// EPT Page Directory Pointer Table Entry
//
struct ept_pdpte_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t reserved_1 : 5;
      uint64_t accessed : 1;
      uint64_t reserved_2 : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_3 : 1;
      uint64_t page_frame_number : 36;
    };
  };
};

//
// EPT Large Page Directory Pointer Table Entry
//
struct ept_pdpte_large_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t memory_type : 3;
      uint64_t ignore_pat : 1;
      uint64_t large_page : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_1 : 19;
      uint64_t page_frame_number : 18;
      uint64_t reserved_2 : 15;
      uint64_t suppress_ve : 1;
    };
  };
};

//
// EPT Page Map Level 4 Entry
//
// Must be aligned to 4kb boundary.
//
struct ept_pml4e_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t reserved_1 : 5;
      uint64_t accessed : 1;
      uint64_t reserved_2 : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_3 : 1;
      uint64_t page_frame_number : 36;
    };
  };
};

//
// EPT Pointer structure
//
struct ept_ptr_t
{
  static constexpr int page_walk_length_4 = 3;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t memory_type : 3;
      uint64_t page_walk_length : 3;
      uint64_t enable_access_and_dirty_flags : 1;
      uint64_t reserved_1 : 5;
      uint64_t page_frame_number : 36;
    };
  };
};

struct ept_descriptor_tag : page_descriptor_tag {};

//
// Page Table type descriptor
//
struct ept_pt_t
{
  using        descriptor_tag = ept_descriptor_tag;
  using                 entry = ept_pte_t;
  static constexpr auto level = pml::pt;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = uint64_t(12);           // 12 = (first set bit of 4kb) - 1
  static constexpr auto size  = uint64_t(1) << shift;   // 4kb
  static constexpr auto mask  = ~(size - 1);
};

//
// EPT Page Directory type descriptor
//
struct ept_pd_t
{
  using        descriptor_tag = ept_descriptor_tag;
  using                 entry = ept_pde_t;
  static constexpr auto level = pml::pd;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = ept_pt_t::shift + 9;    // 21
  static constexpr auto size  = uint64_t(1) << shift;   // 2MB
  static constexpr auto mask  = ~(size - 1);
};

//
// EPT Page Directory Pointer Table type descriptor
//
struct ept_pdpt_t
{
  using        descriptor_tag = ept_descriptor_tag;
  using                 entry = ept_pdpte_t;
  static constexpr auto level = pml::pdpt;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = ept_pd_t::shift + 9;    // 30
  static constexpr auto size  = uint64_t(1) << shift;   // 1GB
  static constexpr auto mask  = ~(size - 1);
};

//
// EPT Page Map Level 4 type descriptor
//
struct ept_pml4_t
{
  using        descriptor_tag = ept_descriptor_tag;
  using                 entry = ept_pml4e_t;
  static constexpr auto level = pml::pml4;
  static constexpr auto count = uint64_t(512);
  static constexpr auto shift = ept_pdpt_t::shift + 9;  // 39
  static constexpr auto size  = uint64_t(1) << shift;   // 512GB
  static constexpr auto mask  = ~(size - 1);
};

//
// EPT entry and common fields
//

struct epte_t
{
  enum class access_type : uint32_t
  {
    none               = 0b0000'0000,
    read               = 0b0000'0001,
    write              = 0b0000'0010,
    execute            = 0b0000'0100,

    read_write         = read  |  write,
    read_execute       = read  | execute,
    read_write_execute = read  |  write  | execute,
    write_execute      = write | execute,

    access_mask        = 0b0000'0111,
  };

  union
  {
    uint64_t          flags;

    ept_pml4e_t       pml4;
    ept_pdpte_large_t pdpt_large; // 1GB
    ept_pdpte_t       pdpt;
    ept_pde_large_t   pd_large;   // 2MB
    ept_pde_t         pd;
    ept_pte_t         pt;

    //
    // Common fields.
    //

    struct
    {
      uint64_t read_access : 1;
      uint64_t write_access : 1;
      uint64_t execute_access : 1;
      uint64_t memory_type : 3;
      uint64_t ignore_pat : 1;
      uint64_t large_page : 1;
      uint64_t accessed : 1;
      uint64_t dirty : 1;
      uint64_t user_mode_execute : 1;
      uint64_t reserved_1 : 1;
      uint64_t page_frame_number : 36;
      uint64_t reserved_2 : 15;
      uint64_t suppress_ve : 1;
    };

    struct
    {
      uint64_t access : 3; // bits of read_access
                           //         write_access
                           //         execute_access
    };
  };

  void clear() noexcept
  {
    flags = 0;
  }

  void update(access_type new_access) noexcept
  {
    access = uint64_t(new_access);
  }

  void update(pa_t pa,
              access_type new_access = access_type::read_write_execute) noexcept
  {
    update(new_access);
    page_frame_number = pa.pfn();
  }

  void update(pa_t pa,
              ia32::memory_type type,
              access_type new_access = access_type::read_write_execute) noexcept
  {
    update(pa, new_access);
    memory_type = static_cast<uint64_t>(type);
  }

  void update(pa_t pa,
              ia32::memory_type type,
              bool large,
              access_type new_access = access_type::read_write_execute) noexcept
  {
    update(pa, type, new_access);
    large_page = large;
  }

  epte_t* subtable() const noexcept
  {
    return present()
      ? reinterpret_cast<epte_t*>(pa_t::from_pfn(page_frame_number).va())
      : nullptr;
  }

  bool present() const noexcept
  {
    return read_access || write_access || execute_access;
  }
};

static_assert(sizeof(epte_t) == 8);

constexpr inline epte_t::access_type operator&(epte_t::access_type value1, epte_t::access_type value2) noexcept
{ return static_cast<epte_t::access_type>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

constexpr inline epte_t::access_type operator|(epte_t::access_type value1, epte_t::access_type value2) noexcept
{ return static_cast<epte_t::access_type>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

constexpr inline epte_t::access_type& operator&=(epte_t::access_type& value1, epte_t::access_type value2) noexcept
{ value1 = value1 & value2; return value1; }

constexpr inline epte_t::access_type& operator|=(epte_t::access_type& value1, epte_t::access_type value2) noexcept
{ value1 = value1 | value2; return value1; }

}
