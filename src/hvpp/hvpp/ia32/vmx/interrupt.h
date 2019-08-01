#pragma once
#include <cstdint>

namespace ia32::vmx {

enum class interrupt_type : uint32_t
{
  external              = 0,
  reserved              = 1,
  nmi                   = 2,
  hardware_exception    = 3,
  software              = 4,
  privileged_exception  = 5,
  software_exception    = 6,
  other_event           = 7,
};

struct interrupt_info_t
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t vector : 8;
      uint32_t type : 3;
      uint32_t error_code_valid : 1;
      uint32_t nmi_unblocking : 1; // Used only in VMEXIT interruption-information,
                                   // otherwise reserved.
      uint32_t reserved : 18;
      uint32_t valid : 1;
    };
  };
};

struct interruptibility_state_t
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t blocking_by_sti : 1;
      uint32_t blocking_by_mov_ss : 1;
      uint32_t blocking_by_smi : 1;
      uint32_t blocking_by_nmi : 1;
      uint32_t enclave_interruption : 1;
      uint32_t reserved : 27;
    };
  };
};

constexpr inline const char* to_string(interrupt_type value) noexcept
{
  switch (value)
  {
    case interrupt_type::external: return "external";
    case interrupt_type::reserved: return "reserved";
    case interrupt_type::nmi: return "nmi";
    case interrupt_type::hardware_exception: return "hardware_exception";
    case interrupt_type::software: return "software";
    case interrupt_type::privileged_exception: return "privileged_exception";
    case interrupt_type::software_exception: return "software_exception";
    case interrupt_type::other_event: return "other_event";
  }

  return "";
}

}
