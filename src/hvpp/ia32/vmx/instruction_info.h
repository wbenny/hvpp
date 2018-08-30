#pragma once
#include <cstdint>

namespace ia32::vmx {

struct instruction_info_ins_outs_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 7;
      uint64_t address_size : 3;
      uint64_t reserved_2 : 5;
      uint64_t segment_register : 3;
    };
  };
};

struct instruction_info_invalidate_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved_1 : 5;
      uint64_t address_size : 3;
      uint64_t reserved_2 : 5;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
      uint64_t register_2 : 4;
    };
  };
};

struct instruction_info_gdtr_idtr_access_t
{
  enum
  {
    instruction_sgdt = 0,
    instruction_sidt = 1,
    instruction_lgdt = 2,
    instruction_lidt = 3,
  };

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved1 : 5;
      uint64_t address_size : 3;
      uint64_t reserved2 : 1;
      uint64_t operand_size : 1;
      uint64_t reserved3 : 3;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
      uint64_t instruction : 2;
    };
  };
};

struct instruction_info_ldtr_tr_access_t
{
  enum
  {
    instruction_sldt = 0,
    instruction_str  = 1,
    instruction_lldt = 2,
    instruction_ltr  = 3,
  };

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved_1 : 1;
      uint64_t register_1 : 4;
      uint64_t address_size : 3;
      uint64_t access_type : 1;
      uint64_t reserved_2 : 4;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
      uint64_t instruction : 2;
    };
  };
};

struct instruction_info_rdrand_rdseed_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 3;
      uint64_t destination_register : 4;
      uint64_t reserved_2 : 4;
      uint64_t operand_size : 2;
    };
  };
};

struct instruction_info_vmx_and_xsaves_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved_1 : 5;
      uint64_t address_size : 3;
      uint64_t reserved_2 : 5;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
    };
  };
};

struct instruction_info_vmread_vmwrite_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved_1 : 1;
      uint64_t register_1 : 4;
      uint64_t address_size : 3;
      uint64_t access_type : 1;
      uint64_t reserved_2 : 4;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
      uint64_t register_2 : 4;
    };
  };
};

struct instruction_info_common_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t scaling : 2;
      uint64_t reserved_1 : 5;
      uint64_t address_size : 3;
      uint64_t access_type : 1;
      uint64_t reserved_2 : 4;
      uint64_t segment_register : 3;
      uint64_t index_register : 4;
      uint64_t index_register_invalid : 1;
      uint64_t base_register : 4;
      uint64_t base_register_invalid : 1;
    };
  };
};

struct instruction_info_t
{
  enum
  {
    no_scaling = 0,
    scale_by_2 = 1,
    scale_by_4 = 2,
    scale_by_8 = 3,
  };

  enum
  {
    size_16bit = 0,
    size_32bit = 1,
    size_64bit = 2,
  };

  enum
  {
    access_memory = 0,
    access_register = 1,
  };

  static constexpr uint64_t size_to_mask[] = {
    0xFFFFull,
    0xFFFFFFFFull,
    0xFFFFFFFFFFFFFFFFull,
  };

  union
  {
    uint64_t flags;

    instruction_info_ins_outs_t         ins_outs;
    instruction_info_invalidate_t       invalidate;
    instruction_info_gdtr_idtr_access_t gdtr_idtr_access;
    instruction_info_ldtr_tr_access_t   ldtr_tr_access;
    instruction_info_rdrand_rdseed_t    rdrand_rdseed;
    instruction_info_vmx_and_xsaves_t   vmx_and_xsaves;
    instruction_info_vmread_vmwrite_t   vmread_vmwrite;

    //
    // Custom "common" field.
    //
    instruction_info_common_t           common;
  };
};

inline constexpr char* instruction_info_gdtr_idtr_to_string(uint64_t value) noexcept
{
  switch (value)
  {
    case instruction_info_gdtr_idtr_access_t::instruction_sgdt: return "sgdt";
    case instruction_info_gdtr_idtr_access_t::instruction_sidt: return "sidt";
    case instruction_info_gdtr_idtr_access_t::instruction_lgdt: return "lgdt";
    case instruction_info_gdtr_idtr_access_t::instruction_lidt: return "lidt";
    default: return "";
  }
}


inline constexpr char* instruction_info_ldtr_tr_to_string(uint64_t value) noexcept
{
  switch (value)
  {
    case instruction_info_ldtr_tr_access_t::instruction_sldt: return "sldt";
    case instruction_info_ldtr_tr_access_t::instruction_str:  return "str";
    case instruction_info_ldtr_tr_access_t::instruction_lldt: return "lldt";
    case instruction_info_ldtr_tr_access_t::instruction_ltr:  return "ltr";
    default: return "";
  }
}

}
