#include "../memory.h"

extern "C"
{
  uint64_t ia32_detail_pa_from_va(void* va);
  void*    ia32_detail_va_from_pa(uint64_t);
  uint64_t ia32_detail_check_physical_memory(void* range_list, int range_list_size, int* count);
}

namespace ia32::detail
{
  uint64_t pa_from_va(void* va) noexcept
  {
    return ia32_detail_pa_from_va(va);
  }

  void* va_from_pa(uint64_t pa) noexcept
  {
    return ia32_detail_va_from_pa(pa);
  }

  void check_physical_memory(memory_range* range_list, int range_list_size, int& count) noexcept
  {
    ia32_detail_check_physical_memory(range_list, range_list_size, &count);
  }
}
