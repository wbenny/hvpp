#pragma once
#include "../memory_allocator.h"

#include "../../bitmap.h"
#include "../../object.h"
#include "../../spinlock.h"

namespace mm
{
  class hypervisor_memory_allocator
    : public memory_allocator
  {
    public:
      hypervisor_memory_allocator() noexcept;
      ~hypervisor_memory_allocator() noexcept override;

      auto attach(void* address, size_t size) noexcept -> error_code_t override;
      void detach() noexcept override;

      auto allocate(size_t size) noexcept -> void* override;
      auto allocate_aligned(size_t size, size_t alignment) noexcept -> void* override;
      void free(void* address) noexcept override;

      bool contains(void* address) noexcept override;

      auto allocated_bytes() noexcept -> size_t override;
      auto free_bytes() noexcept -> size_t override;

    private:
      using pgbmp_t = bitmap<>;
      using pgmap_t = uint16_t;

      uint8_t*    base_address_;               // Pool base address
      size_t      capacity_;                   // Capacity of the pool

      pgbmp_t     page_bitmap_;                // Bitmap holding used pages
      int         page_bitmap_buffer_size_;    //

      pgmap_t*    page_allocation_map_;        // Map holding number of allocated pages
      int         page_allocation_map_size_;   //

      int         last_page_offset_;           // Last returned page offset - used as hint

      size_t      allocated_bytes_;
      size_t      free_bytes_;

      spinlock    lock_;
  };
}
