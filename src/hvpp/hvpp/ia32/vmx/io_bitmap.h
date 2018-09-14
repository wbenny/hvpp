#pragma once
#include "../memory.h"

#include <cstdint>

namespace ia32::vmx {

struct alignas(page_size) io_bitmap_t
{
  static constexpr uint32_t io_bitmap_a_min = 0x00000000;
  static constexpr uint32_t io_bitmap_a_max = 0x00007fff;
  static constexpr uint32_t io_bitmap_b_min = 0x00008000;
  static constexpr uint32_t io_bitmap_b_max = 0x0000ffff;

  union
  {
    struct
    {
      uint8_t a[page_size];
      uint8_t b[page_size];
    };

    uint8_t data[2 * page_size];
  };
};

static_assert(sizeof(io_bitmap_t) == 2 * page_size);

}

