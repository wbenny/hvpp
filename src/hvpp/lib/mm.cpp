#pragma once
#include "mm.h"
#include "lib/assert.h"
#include "lib/bitmap.h"
#include "lib/spinlock.h"

#include <cstring>
#include <limits>
#include <mutex>

#include <ntddk.h>

//
// Simple memory manager implementation.
//
// Memory manager is provided memory space on which it can operate. Small part
// from this space is reserved for the page bitmap and page allocation map.
//
// Page bitmap sets bit 1 at page offset, if the page is allocated (e.g.:
// if 4th page (at base_address + 4*PAGE_SIZE) is allocated, 4th bit in this
// bitmap is set). On deallocation, corresponding bit is reset to 0.
//
// Page allocation map stores number of pages allocated for the particular
// address (e.g.: allocate(8192) returned (base_address + 4*PAGE_SIZE), which
// is 2 pages, therefore page_allocation_map[4] == 2. On deallocation,
// corresponding number in the map is reset to 0.
//
// Note: allocations are always page-aligned - therefore allocation for
//       even 1 byte results in waste of 4096 bytes.
//

namespace memory_manager
{
  uint8_t*  base_address   = nullptr;         // Pool base address
  size_t    available_size = 0;               // Available memory in the pool

  bitmap*   page_bitmap    = nullptr;         // Bitmap holding used pages
  int       page_bitmap_object_size = 0;      //
  int       page_bitmap_buffer_size = 0;      //

  using pgmap_t = uint16_t;
  pgmap_t*  page_allocation_map = nullptr;    // Map holding number of allocated pages
  int       page_allocation_map_size = 0;     //

  int       last_page_offset = 0;             // Last returned page offset - used as a hint

  size_t    number_of_allocated_bytes = 0;
  size_t    number_of_free_bytes = 0;

  //
  // Well, isn't this unfortunate... If we took the pretty path and just used:
  //   spinlock lock;
  //
  // the linker would complain with:
  //   warning LNK4210: .CRT section exists; there may be unhandled static initializers
  //   or terminators
  //
  // In another words, in Windows Drivers, there is no CRT code to initialize static and
  // global variables with non-trivial construction (which spinlock isn't). But since we
  // know that spinlock consists of 32 bits, we can basically create storage for it with
  // uint32_t and create lock as a reference to that memory.
  //
  // Note that initialization of the spinlock happens in the initialize() function.
  //

  uint32_t  __lock_storage__;
  spinlock& lock = (spinlock&)__lock_storage__;
  static_assert(sizeof(uint32_t) == sizeof(spinlock));

  void initialize(void* address, size_t size) noexcept
  {
    if (size < PAGE_SIZE * 4)
    {
      //
      // We need at least 4 pages (see explanation below).
      //
      hvpp_assert(0);
      return;
    }

    //
    // If the provided address is not page aligned, align it to the next page.
    //
    if (BYTE_OFFSET(address) != 0)
    {
      uint32_t lost_bytes = BYTE_OFFSET(address);

      address = reinterpret_cast<uint8_t*>(PAGE_ALIGN(address)) + PAGE_SIZE;

      //
      // Subtract amount of "lost" bytes due to alignment.
      //
      size -= lost_bytes;
    }

    //
    // Align size to the page boundary.
    //
    size = reinterpret_cast<size_t>(PAGE_ALIGN(size));

    //
    // Check again.
    //
    if (size < PAGE_SIZE * 4)
    {
      hvpp_assert(0);
      return;
    }

    //
    // Address is page-aligned, size is page-aligned, and all requirements are met.
    // Proceed with initialization.
    //

    //
    // The provided memory is split up to 4 parts:
    //   1. page bitmap - stores information if page is allocated or not
    //   2. page bitmap object - stores the bitmap object itself
    //   3. page count  - stores information how many consecutive pages has been allocated
    //   4. memory pool - this is the memory which will be provided
    //
    // For (1), there is taken (size / PAGE_SIZE / 8) bytes from the provided memory space.
    // For (2), there is taken (PAGE_SIZE) bytes from the provided memory space.
    // For (3), there is taken (size / PAGE_SIZE * sizeof(pgmap_t)) bytes from the provided
    // memory space. The rest memory is used for (4). This should account for ~93% of the
    // provided memory space (if it is big enough, e.g.: 32MB).
    //

    static_assert(sizeof(bitmap) < PAGE_SIZE);

    //
    // Construct the page bitmap.
    //
    uint8_t* page_bitmap_buffer = reinterpret_cast<uint8_t*>(address);
    page_bitmap_buffer_size = static_cast<int>(ROUND_TO_PAGES(size / PAGE_SIZE / 8));
    memset(page_bitmap_buffer, 0, page_bitmap_object_size);

    page_bitmap = reinterpret_cast<bitmap*>(page_bitmap_buffer + page_bitmap_buffer_size);
    page_bitmap_object_size = PAGE_SIZE;
    new (page_bitmap) bitmap(page_bitmap_buffer, page_bitmap_buffer_size * 8);

    //
    // Construct the page allocation map.
    //
    page_allocation_map = reinterpret_cast<pgmap_t*>(reinterpret_cast<uint8_t*>(page_bitmap) + PAGE_SIZE);
    page_allocation_map_size = static_cast<int>(ROUND_TO_PAGES(size / PAGE_SIZE) * sizeof(pgmap_t));
    memset(page_allocation_map, 0, page_allocation_map_size);

    //
    // Construct the lock.
    //
    new (&lock) spinlock();

    //
    // Compute available memory.
    //
    base_address = reinterpret_cast<uint8_t*>(page_allocation_map) + page_allocation_map_size;
    available_size = size
      - page_bitmap_buffer_size
      - page_bitmap_object_size
      - page_allocation_map_size;


    number_of_allocated_bytes = 0;
    number_of_free_bytes = size;
  }

  void destroy() noexcept
  {
    std::lock_guard _(lock);

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

    page_bitmap = nullptr;
    page_bitmap_object_size = 0;
    page_bitmap_buffer_size = 0;

    page_allocation_map = nullptr;
    page_allocation_map_size = 0;

    last_page_offset = 0;
    number_of_allocated_bytes = 0;
    number_of_free_bytes = 0;
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

    int page_count = static_cast<int>(BYTES_TO_PAGES(size));

    //
    // Check if the desired number of pages can fit into the allocation map.
    //
    if (page_count > std::numeric_limits<pgmap_t>::max() - 1)
    {
      hvpp_assert(0);
      return nullptr;
    }

    int previous_page_offset;

    {
      std::lock_guard _(lock);

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

      number_of_allocated_bytes += page_count * PAGE_SIZE;
      number_of_free_bytes -= page_count * PAGE_SIZE;
    }

    //
    // Clear the memory. Note that we don't need to be locked to do this.
    //
    auto result = base_address + previous_page_offset * PAGE_SIZE;
    memset(result, 0, size);

    return result;
  }

  void free(void* address) noexcept
  {
    //
    // Our allocator always provides page-aligned memory.
    //
    hvpp_assert(BYTE_OFFSET(address) == 0);

    int offset = static_cast<int>(BYTES_TO_PAGES(reinterpret_cast<uint8_t*>(address) - base_address));

    if (offset * PAGE_SIZE > available_size)
    {
      //
      // We don't own this memory.
      //
      hvpp_assert(0);
      return;
    }

    std::lock_guard _(lock);

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

    number_of_allocated_bytes -= page_count * PAGE_SIZE;
    number_of_free_bytes += page_count * PAGE_SIZE;
  }

  size_t allocated_bytes() noexcept
  {
    return number_of_allocated_bytes;
  }

  size_t free_bytes() noexcept
  {
    return number_of_free_bytes;
  }
}

//////////////////////////////////////////////////////////////////////////

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
