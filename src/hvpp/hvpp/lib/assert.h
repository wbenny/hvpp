#pragma once
#include "debugger.h"

#define hvpp_assert(expression)                         \
  do                                                    \
  {                                                     \
    if (!(expression)) {                                \
      debugger::breakpoint();                           \
    }                                                   \
  } while (0)
