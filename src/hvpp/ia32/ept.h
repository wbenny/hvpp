#pragma once
#include "memory.h"

#include <cstdint>

namespace ia32 {

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

struct ept_pdpte_1gb_t
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

struct ept_pde_2mb_t
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

struct epte_t
{
  enum class access_type : uint32_t
  {
    read        = 0b0000'0001,
    write       = 0b0000'0010,
    execute     = 0b0000'0100,

    read_write         = read | write,
    read_execute       = read | execute,
    read_write_execute = read | write | execute,
    write_execute      = write | execute,

    access_mask = 0b0000'0111,
  };

  union
  {
    uint64_t        flags;

    ept_pml4e_t     pml4;
    ept_pdpte_1gb_t pdpt_1gb;
    ept_pdpte_t     pdpt;
    ept_pde_2mb_t   pd_2mb;
    ept_pde_t       pd;
    ept_pte_t       pt;

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
      access_type access : 3;
    };
  };

  void update(access_type new_access) noexcept
  {
    access = new_access;
  }

  void update(pa_t pa, access_type new_access = access_type::read_write_execute) noexcept
  {
    update(new_access);
    page_frame_number = pa.pfn();
  }

  void update(pa_t pa, ia32::memory_type type, access_type new_access = access_type::read_write_execute) noexcept
  {
    update(pa, new_access);
    memory_type = static_cast<uint64_t>(type);
  }

  void update(pa_t pa, ia32::memory_type type, bool large, access_type new_access = access_type::read_write_execute) noexcept
  {
    update(pa, type, new_access);
    large_page = large;
  }

  epte_t* subtable() const noexcept
  {
    return is_present()
      ? reinterpret_cast<epte_t*>(pa_t::from_pfn(page_frame_number).va())
      : nullptr;
  }

  bool is_present() const noexcept
  {
    return read_access || write_access || execute_access;
  }
};

constexpr inline epte_t::access_type operator&(epte_t::access_type value1, epte_t::access_type value2) noexcept
{ return static_cast<epte_t::access_type>(static_cast<uint32_t>(value1) & static_cast<uint32_t>(value2)); }

constexpr inline epte_t::access_type operator|(epte_t::access_type value1, epte_t::access_type value2) noexcept
{ return static_cast<epte_t::access_type>(static_cast<uint32_t>(value1) | static_cast<uint32_t>(value2)); }

constexpr inline epte_t::access_type& operator&=(epte_t::access_type& value1, epte_t::access_type value2) noexcept
{ value1 = value1 & value2; return value1; }

constexpr inline epte_t::access_type& operator|=(epte_t::access_type& value1, epte_t::access_type value2) noexcept
{ value1 = value1 | value2; return value1; }

static_assert(sizeof(epte_t) == 8);

}
