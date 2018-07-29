#pragma once
#include <cstdint>

namespace ia32::vmx {

struct io_bitmap
{
  static constexpr uint32_t io_bitmap_a_min = 0x00000000;
  static constexpr uint32_t io_bitmap_a_max = 0x00007fff;
  static constexpr uint32_t io_bitmap_b_min = 0x00008000;
  static constexpr uint32_t io_bitmap_b_max = 0x0000ffff;

  union
  {
    struct
    {
      uint8_t a[PAGE_SIZE];
      uint8_t b[PAGE_SIZE];
    };

    uint8_t data[2 * PAGE_SIZE];
  };
};

static_assert(sizeof(io_bitmap) == 2 * PAGE_SIZE);

}

