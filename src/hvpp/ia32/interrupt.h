#pragma once
#include "ia32/asm.h"

namespace ia32::interrupts {

inline void enable()  { ia32_asm_enable_interrupts(); }
inline void disable() { ia32_asm_disable_interrupts(); }

}
