#pragma once
#include <cstdint>

namespace ia32::detail {

uint64_t pa_from_va(void* va) noexcept;
void*    va_from_pa(uint64_t pa) noexcept;

}
