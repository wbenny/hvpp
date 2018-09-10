#include "mm.h"

#include "ia32/memory.h"

#include "lib/assert.h"
#include "lib/bitmap.h"
#include "lib/object.h"
#include "lib/spinlock.h"

#include <cstring>
#include <limits>
#include <mutex>

//
// Simple memory manager implementation.
//
// Because in VM-exits it is very dangerous to call OS
// functions for memory (de)allocation (they can cause
// IPIs and/or TLB flush), the hypervisor has its own
// simple memory manager.  The memory manager should be
// the very first thing to initialize.
//
// Memory manager is provided memory space on which it
// can operate.  Small part from this space is reserved
// for the page bitmap and page allocation map.
//
// Page bitmap sets bit 1 at page offset, if the page is
// allocated (e.g.: if 4th page (at base_address + 4*PAGE_SIZE)
// is allocated, 4th bit in this bitmap is set).
// On deallocation, corresponding bit is reset to 0.
//
// Page allocation map stores number of pages allocated
// for the particular address (e.g.: allocate(8192) returned
// (base_address + 4*PAGE_SIZE), which is 2 pages, therefore
// page_allocation_map[4] == 2.
// On deallocation, corresponding number in the map is reset
// to 0.
//
// Note: allocations are always page-aligned - therefore
//       allocation for even 1 byte results in waste of
//       4096 bytes.
//

namespace memory_manager
{
  uint8_t*  base_address = nullptr;         // Pool base address
  size_t    available_size = 0;             // Available memory in the pool

  using pgbmp_t = object_t<bitmap>;
  pgbmp_t   page_bitmap;                    // Bitmap holding used pages
  int       page_bitmap_buffer_size = 0;    //

  using pgmap_t = uint16_t;
  pgmap_t*  page_allocation_map = nullptr;  // Map holding number of allocated pages
  int       page_allocation_map_size = 0;   //

  int       last_page_offset = 0;           // Last returned page offset - used as hint

  size_t    number_of_allocated_bytes = 0;
  size_t    number_of_free_bytes = 0;

  object_t<ia32::physical_memory_descriptor> memory_descriptor;
  object_t<ia32::mtrr> memory_type_range_registers;
  object_t<spinlock> lock;

  auto initialize() noexcept -> error_code_t
  {
    //
    // Initialize physical memory descriptor and MTRRs.
    //
    memory_descriptor.initialize();
    memory_type_range_registers.initialize();

    //
    // Initialize lock.
    //
    lock.initialize();

    return error_code_t{};
  }

  void destroy() noexcept
  {
    //
    // Destroy all objects.
    // Note that this method doesn't acquire the lock and
    // assumes all allocations has been already freed.
    //
    memory_type_range_registers.destroy();
    memory_descriptor.destroy();
    lock.destroy();

    //
    // If no memory has been assigned - leave.
    //
    if (!base_address)
    {
      return;
    }

    //
    // Mark memory of page_bitmap and page_allocation_map
    // as freed.
    //
    // Note that everything "free" does is clear bits in
    // page_bitmap and sets 0 to particular page_allocation_map
    // items.
    //
    // These two calls are needed to assure that the next two
    // asserts below will pass.
    //
    free(page_bitmap->buffer());
    free(page_allocation_map);

    //
    // Checks for memory leaks.
    //
    hvpp_assert(page_bitmap->all_clear());

    //
    // Checks for allocator corruption.
    //
    hvpp_assert(std::all_of(
      page_allocation_map,
      page_allocation_map + page_allocation_map_size / sizeof(pgmap_t),
      [](auto page_count) { return page_count == 0; }));

    base_address = nullptr;
    available_size = 0;

    page_bitmap.destroy();
    page_bitmap_buffer_size = 0;

    page_allocation_map = nullptr;
    page_allocation_map_size = 0;

    last_page_offset = 0;
    number_of_allocated_bytes = 0;
    number_of_free_bytes = 0;
  }

  auto assign(void* address, size_t size) noexcept -> error_code_t
  {
    if (size < ia32::page_size * 3)
    {
      //
      // We need at least 3 pages (see explanation below).
      //
      hvpp_assert(0);
      return make_error_code_t(std::errc::invalid_argument);
    }

    //
    // If the provided address is not page aligned, align it
    // to the next page.
    //
    if (ia32::byte_offset(address) != 0)
    {
      uint32_t lost_bytes = ia32::byte_offset(address);

      address = reinterpret_cast<uint8_t*>(ia32::page_align(address)) + ia32::page_size;

      //
      // Subtract amount of "lost" bytes due to alignment.
      //
      size -= lost_bytes;
    }

    //
    // Align size to the page boundary.
    //
    size = ia32::page_align(size);

    //
    // Check again.
    //
    if (size < ia32::page_size * 3)
    {
      hvpp_assert(0);
      return make_error_code_t(std::errc::invalid_argument);
    }

    //
    // Address is page-aligned, size is page-aligned, and all
    // requirements are met.  Proceed with initialization.
    //

    //
    // The provided memory is split up to 3 parts:
    //   1. page bitmap - stores information if page is allocated
    //      or not
    //   2. page count  - stores information how many consecutive
    //      pages has been allocated
    //   3. memory pool - this is the memory which will be provided
    //
    // For (1), there is taken (size / PAGE_SIZE / 8) bytes from the
    //          provided memory space.
    // For (2), there is taken (size / PAGE_SIZE * sizeof(pgmap_t))
    //          bytes from the provided memory space.
    // The rest memory is used for (3).
    //
    // This should account for ~93% of the provided memory space (if
    // it is big enough, e.g.: 32MB).
    //

    //
    // Construct the page bitmap.
    //
    uint8_t* page_bitmap_buffer = reinterpret_cast<uint8_t*>(address);
    page_bitmap_buffer_size = static_cast<int>(ia32::round_to_pages(size / ia32::page_size / 8));
    memset(page_bitmap_buffer, 0, page_bitmap_buffer_size);

    int page_bitmap_size_in_bits = static_cast<int>(size / ia32::page_size);
    page_bitmap.initialize(page_bitmap_buffer, page_bitmap_size_in_bits);

    //
    // Construct the page allocation map.
    //
    page_allocation_map = reinterpret_cast<pgmap_t*>(page_bitmap_buffer + page_bitmap_buffer_size);
    page_allocation_map_size = static_cast<int>(ia32::round_to_pages(size / ia32::page_size) * sizeof(pgmap_t));
    memset(page_allocation_map, 0, page_allocation_map_size);

    //
    // Compute available memory.
    //
    int reserved_bytes = static_cast<int>(page_bitmap_buffer_size + page_allocation_map_size);

    base_address = reinterpret_cast<uint8_t*>(address);
    available_size = size - reserved_bytes;

    //
    // Mark memory of page_bitmap and page_allocation_map as allocated.
    // The return value of these allocations should return the exact
    // address of page_bitmap_buffer and page_allocation_map.
    //
    void* page_bitmap_buffer_tmp  = allocate(page_bitmap_buffer_size);
    void* page_allocation_map_tmp = allocate(page_allocation_map_size);

    hvpp_assert(reinterpret_cast<uintptr_t>(page_bitmap_buffer)  == reinterpret_cast<uintptr_t>(page_bitmap_buffer_tmp));
    hvpp_assert(reinterpret_cast<uintptr_t>(page_allocation_map) == reinterpret_cast<uintptr_t>(page_allocation_map_tmp));

    (void)page_bitmap_buffer_tmp;
    (void)page_allocation_map_tmp;

    //
    // Initialize memory pool with garbage.
    // This should help with debugging uninitialized variables
    // and class members.
    //
    memset(base_address + reserved_bytes, 0xcc, available_size);

    //
    // Set initial values of allocated/free bytes.
    //
    number_of_allocated_bytes = 0;
    number_of_free_bytes = size;

    return error_code_t{};
  }

  void* allocate(size_t size) noexcept
  {
    hvpp_assert(base_address != nullptr && available_size > 0);

    //
    // Return at least 1 page, even if someone required 0.
    //
    if (size == 0)
    {
      hvpp_assert(0);
      size = 1;
    }

    int page_count = static_cast<int>(ia32::bytes_to_pages(size));

    //
    // Check if the desired number of pages can fit into the
    // allocation map.
    //
    if (page_count > std::numeric_limits<pgmap_t>::max() - 1)
    {
      hvpp_assert(0);
      return nullptr;
    }

    int previous_page_offset;

    {
      std::lock_guard _(*lock);

      last_page_offset = page_bitmap->find_first_clear(last_page_offset, page_count);

      if (last_page_offset == -1)
      {
        last_page_offset = 0;
        last_page_offset = page_bitmap->find_first_clear(last_page_offset, page_count);

        if (last_page_offset == -1)
        {
          //
          // Not enough memory...
          //
          hvpp_assert(0);
          return nullptr;
        }
      }

      page_bitmap->set(last_page_offset, page_count);
      page_allocation_map[last_page_offset] = static_cast<pgmap_t>(page_count);

      previous_page_offset = last_page_offset;
      last_page_offset += page_count;

      number_of_allocated_bytes += page_count * ia32::page_size;
      number_of_free_bytes      -= page_count * ia32::page_size;
    }

    //
    // Return the final address.
    // Note that we're not under lock here - we don't need it, because
    // everything neccessary has been done (bitmap + page allocation map
    // manipulation).
    //
    return base_address + previous_page_offset * ia32::page_size;
  }

  void free(void* address) noexcept
  {
    //
    // Our allocator always provides page-aligned memory.
    //
    hvpp_assert(ia32::byte_offset(address) == 0);

    int offset = static_cast<int>(ia32::bytes_to_pages(reinterpret_cast<uint8_t*>(address) - base_address));

    if (size_t(offset) * ia32::page_size > available_size)
    {
      //
      // We don't own this memory.
      //
      hvpp_assert(0);
      return;
    }

    std::lock_guard _(*lock);

    if (page_allocation_map[offset] == 0)
    {
      //
      // This memory wasn't allocated.
      //
      hvpp_assert(0);
      return;
    }

    //
    // Clear number of allocated pages.
    //
    int page_count = page_allocation_map[offset];
    page_allocation_map[offset] = 0;

    //
    // Clear pages in the bitmap.
    //
    page_bitmap->clear(offset, page_count);

    number_of_allocated_bytes -= page_count * ia32::page_size;
    number_of_free_bytes      += page_count * ia32::page_size;
  }

  size_t allocated_bytes() noexcept
  {
    return number_of_allocated_bytes;
  }

  size_t free_bytes() noexcept
  {
    return number_of_free_bytes;
  }

  const ia32::physical_memory_descriptor& physical_memory_descriptor() noexcept
  {
    return *memory_descriptor;
  }

  const ia32::mtrr& mtrr() noexcept
  {
    return *memory_type_range_registers;
  }
}

void* operator new  (size_t size)                                    { return memory_manager::allocate(size); }
void* operator new[](size_t size)                                    { return memory_manager::allocate(size); }
void* operator new  (size_t size, std::align_val_t)                  { return memory_manager::allocate(size); }
void* operator new[](size_t size, std::align_val_t)                  { return memory_manager::allocate(size); }

void operator delete  (void* address)                                { memory_manager::free(address); }
void operator delete[](void* address)                                { memory_manager::free(address); }
void operator delete[](void* address, std::size_t)                   { memory_manager::free(address); }
void operator delete  (void* address, std::size_t)                   { memory_manager::free(address); }
void operator delete  (void* address, std::align_val_t)              { memory_manager::free(address); }
void operator delete[](void* address, std::align_val_t)              { memory_manager::free(address); }
void operator delete[](void* address, std::size_t, std::align_val_t) { memory_manager::free(address); }
void operator delete  (void* address, std::size_t, std::align_val_t) { memory_manager::free(address); }
