#pragma once
#include <cstdint>

namespace ia32::vmx {

struct msr_bitmap
{
  static constexpr uint32_t msr_id_low_min  = 0x00000000;
  static constexpr uint32_t msr_id_low_max  = 0x00001fff;
  static constexpr uint32_t msr_id_high_min = 0xc0000000;
  static constexpr uint32_t msr_id_high_max = 0xc0001fff;

  union
  {
    struct
    {
      uint8_t rdmsr_low [PAGE_SIZE / 4];
      uint8_t rdmsr_high[PAGE_SIZE / 4];
      uint8_t wrmsr_low [PAGE_SIZE / 4];
      uint8_t wrmsr_high[PAGE_SIZE / 4];
    };

    uint8_t data[PAGE_SIZE];
  };

  void set(uint32_t msr_id, bool read = true, bool set = true) noexcept
  {
    uint8_t* ptr = nullptr;
    uint32_t msr_offset = 0;
    if (/*msr_id >= msr_id_low_min &&*/ msr_id <= msr_id_low_max)
    {
      ptr = read ? rdmsr_low : wrmsr_low;
      msr_offset = msr_id;
    }

    if (msr_id >= msr_id_high_min && msr_id <= msr_id_high_max)
    {
      ptr = read ? rdmsr_high : wrmsr_high;
      msr_offset = 0xc0000000 - msr_id;
    }

    if (ptr)
    {
      if (set)
      {
        ptr[msr_offset / 8] |= (1 << (msr_offset % 8));
      }
      else
      {
        ptr[msr_offset / 8] &= ~(1 << (msr_offset % 8));
      }
    }
  }
};

static_assert(sizeof(msr_bitmap) == PAGE_SIZE);

}

