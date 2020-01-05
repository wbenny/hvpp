#pragma once
#include "debugger.h"
#include "../config.h"

#if defined(HVPP_DISABLE_ASSERT)
# define hvpp_assert(expression)
#else
# define hvpp_assert(expression)                        \
  do                                                    \
  {                                                     \
    if (!(expression)) {                                \
      debugger::breakpoint();                           \
    }                                                   \
  } while (0)
#endif
