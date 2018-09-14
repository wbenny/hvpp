#pragma once
#include <cstdint>

namespace ia32 {

struct m128_t
{
  uint64_t low;
  uint64_t high;
};

struct fsave_area_t
{
  uint32_t control_word;
  uint32_t status_word;
  uint32_t tag_word;
  uint32_t ip_offset;
  uint32_t ip_selector;
  uint32_t operand_pointer_offset;
  uint32_t operand_pointer_selector;
  uint32_t st_register[20];
};

struct fxsave_area_t
{
  uint16_t control_word;
  uint16_t status_word;
  uint16_t tag_word;
  uint16_t last_instruction_opcode;

  union
  {
    struct
    {
      uint64_t rip;
      uint64_t rdp;
    };

    struct
    {
      uint32_t ip_offset;
      uint32_t ip_selector;
      uint32_t operand_pointer_offset;
      uint32_t operand_pointer_selector;
    };
  };

  uint32_t mxcsr;
  uint32_t mxcsr_mask;

  m128_t   st_register[8];    // st0-st7 (mm0-mm7), only 80 bits per register
                              // (upper 48 bits of each register are reserved)

#ifdef _WIN64
  m128_t   xmm_register[16];  // xmm0-xmm15
  uint8_t  reserved_1[48];
#else
  m128_t   xmm_register[8];   // xmm0-xmm7
  uint8_t  reserved_1[176];
#endif

  union
  {
    uint8_t reserved_2[48];
    uint8_t software_available[48];
  };
};

static_assert(sizeof(fxsave_area_t) == 512);

struct ymmh_area_t
{
  m128_t ymmh_register[16];
};

static_assert(sizeof(ymmh_area_t) == 256);

struct lwp_area_t
{
  uint8_t reserved[128];
};

static_assert(sizeof(lwp_area_t) == 128);

#pragma pack(push, 1)
struct bndreg_t
{
  uint64_t lower_bound;
  uint64_t upper_bound;
};

struct bndcsr_t
{
  uint64_t bndcfgu;
  uint64_t bndstatus;
};

struct xsave_area_header_t
{
  uint64_t xstate_bv;
  uint64_t xcomp_bv;
  uint64_t reserved[6];
};

struct alignas(64) xsave_area_t
{
  fxsave_area_t       fxsave;
  xsave_area_header_t xsave_header;
  ymmh_area_t         ymmh;
  lwp_area_t          lwp;
  bndreg_t            bndreg[4];
  bndcsr_t            bndcsr;
};
#pragma pack(pop)

}
