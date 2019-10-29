#pragma once
#include "debugger.h"

#if 1
# define hvpp_assert(expression)                        \
  do                                                    \
  {                                                     \
    if (!(expression)) {                                \
      debugger::breakpoint();                           \
    }                                                   \
  } while (0)
#else
# define hvpp_assert(expression)
#endif
