#pragma once
#include "ia32/asm.h"

#define hvpp_assert(expression)                         \
  do                                                    \
  {                                                     \
    if (!(expression)) {                                \
      ia32_asm_int3();                                  \
    }                                                   \
  } while (0)
