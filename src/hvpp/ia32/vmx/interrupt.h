#pragma once
#include <cstdint>

namespace ia32::vmx {

enum class interrupt_type : uint32_t
{
  external                    = 0,
  reserved                    = 1,
  nmi                         = 2,
  hardware_exception          = 3,
  software                    = 4,
  privileged_exception        = 5,
  software_exception          = 6,
  other_event                 = 7,
};

struct interrupt_info
{
  union
  {
    uint32_t flags;

    struct
    {
      uint32_t vector : 8;
      uint32_t type : 3;
      uint32_t error_code_valid : 1;
      uint32_t nmi_unblocking : 1; // Used only in VMEXIT interruption-information, otherwise reserved.
      uint32_t reserved : 18;
      uint32_t valid : 1;
    };
  };
};

}
