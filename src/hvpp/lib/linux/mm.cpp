#include "../mm.h"

extern "C"
{
  void* memory_manager_detail_system_allocate(size_t size);
  void  memory_manager_detail_system_free(void* address);
}

namespace memory_manager::detail
{
  void* system_allocate(size_t size) noexcept
  {
    return memory_manager_detail_system_allocate(size);
  }

  void system_free(void* address) noexcept
  {
    return memory_manager_detail_system_free(address);
  }
}
