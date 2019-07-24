#include "mm.h"

#include "hvpp/ia32/memory.h"
#include "hvpp/config.h"

#include "assert.h"
#include "bitmap.h"
#include "object.h"
#include "spinlock.h"
#include "mp.h"

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

namespace mm
{
  using pgbmp_t = object_t<bitmap>;
  using pgmap_t = uint16_t;

  struct global_t
  {
    uint8_t*    base_address;               // Pool base address
    size_t      available_size;             // Available memory in the pool

    pgbmp_t     page_bitmap;                // Bitmap holding used pages
    int         page_bitmap_buffer_size;    //

    pgmap_t*    page_allocation_map;        // Map holding number of allocated pages
    int         page_allocation_map_size;   //

    int         last_page_offset;           // Last returned page offset - used as hint

    size_t      allocated_bytes;
    size_t      free_bytes;

    allocator_t allocator[HVPP_MAX_CPU];

    object_t<ia32::physical_memory_descriptor> memory_descriptor;
    object_t<ia32::mtrr> memory_type_range_registers;
    object_t<spinlock> lock;
  };

  global_t global;

  const allocator_t system_allocator = { &system_allocate, &system_free };
  const allocator_t custom_allocator = { &allocate,        &free        };

  allocator_guard::allocator_guard() noexcept
    : allocator_guard(custom_allocator)
  {

  }

  allocator_guard::allocator_guard(const allocator_t& new_allocator) noexcept
    : previous_allocator_(allocator())
  {
    allocator(new_allocator);
  }

  allocator_guard::~allocator_guard() noexcept
  {
    allocator(previous_allocator_);
  }

  auto initialize() noexcept -> error_code_t
  {
    //
    // Initialize physical memory descriptor and MTRRs.
    //
    global.memory_descriptor.initialize();
    global.memory_type_range_registers.initialize();

    //
    // Initialize lock.
    //
    global.lock.initialize();

    //
    // Set system allocator by default.
    //
    for (auto& allocator_item : global.allocator)
    {
      allocator_item = system_allocator;
    }

    return error_code_t{};
  }

  void destroy() noexcept
  {
    //
    // Destroy all objects.
    // Note that this method doesn't acquire the lock and
    // assumes all allocations has been already freed.
    //
    global.memory_type_range_registers.destroy();
    global.memory_descriptor.destroy();
    global.lock.destroy();

    //
    // If no memory has been assigned - leave.
    //
    if (!global.base_address)
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
    free(global.page_bitmap->buffer());
    free(global.page_allocation_map);

    //
    // Checks for memory leaks.
    //
    hvpp_assert(global.page_bitmap->all_clear());

    //
    // Checks for allocator corruption.
    //
    hvpp_assert(std::all_of(
      global.page_allocation_map,
      global.page_allocation_map + global.page_allocation_map_size / sizeof(pgmap_t),
      [](auto page_count) { return page_count == 0; }));

    global.base_address = nullptr;
    global.available_size = 0;

    global.page_bitmap.destroy();
    global.page_bitmap_buffer_size = 0;

    global.page_allocation_map = nullptr;
    global.page_allocation_map_size = 0;

    global.last_page_offset = 0;
    global.allocated_bytes = 0;
    global.free_bytes = 0;
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
      const auto lost_bytes = ia32::byte_offset(address);

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
    global.page_bitmap_buffer_size = static_cast<int>(ia32::round_to_pages(size / ia32::page_size / 8));
    memset(page_bitmap_buffer, 0, global.page_bitmap_buffer_size);

    int page_bitmap_size_in_bits = static_cast<int>(size / ia32::page_size);
    global.page_bitmap.initialize(page_bitmap_buffer, page_bitmap_size_in_bits);

    //
    // Construct the page allocation map.
    //
    global.page_allocation_map = reinterpret_cast<pgmap_t*>(page_bitmap_buffer + global.page_bitmap_buffer_size);
    global.page_allocation_map_size = static_cast<int>(ia32::round_to_pages(size / ia32::page_size) * sizeof(pgmap_t));
    memset(global.page_allocation_map, 0, global.page_allocation_map_size);

    //
    // Compute available memory.
    //
    global.base_address = reinterpret_cast<uint8_t*>(address);
    global.available_size = size;

    //
    // Mark memory of page_bitmap and page_allocation_map as allocated.
    // The return value of these allocations should return the exact
    // address of page_bitmap_buffer and page_allocation_map.
    //
    void* page_bitmap_buffer_tmp  = allocate(global.page_bitmap_buffer_size);
    void* page_allocation_map_tmp = allocate(global.page_allocation_map_size);

    hvpp_assert(reinterpret_cast<uintptr_t>(       page_bitmap_buffer)  == reinterpret_cast<uintptr_t>(page_bitmap_buffer_tmp));
    hvpp_assert(reinterpret_cast<uintptr_t>(global.page_allocation_map) == reinterpret_cast<uintptr_t>(page_allocation_map_tmp));

    (void)(page_bitmap_buffer_tmp);
    (void)(page_allocation_map_tmp);

    //
    // Initialize memory pool with garbage.
    // This should help with debugging uninitialized variables
    // and class members.
    //
    const auto reserved_bytes = static_cast<int>(global.page_bitmap_buffer_size + global.page_allocation_map_size);
    memset(global.base_address + reserved_bytes, 0xcc, size - reserved_bytes);

    //
    // Set initial values of allocated/free bytes.
    //
    global.allocated_bytes = 0;
    global.free_bytes = size;

    return error_code_t{};
  }

  auto allocate(size_t size) noexcept -> void*
  {
    hvpp_assert(global.base_address != nullptr && global.available_size > 0);

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
      std::lock_guard _{ *global.lock };

      global.last_page_offset = global.page_bitmap->find_first_clear(global.last_page_offset, page_count);

      if (global.last_page_offset == -1)
      {
        global.last_page_offset = 0;
        global.last_page_offset = global.page_bitmap->find_first_clear(global.last_page_offset, page_count);

        if (global.last_page_offset == -1)
        {
          //
          // Not enough memory...
          //
          hvpp_assert(0);
          return nullptr;
        }
      }

      global.page_bitmap->set(global.last_page_offset, page_count);
      global.page_allocation_map[global.last_page_offset] = static_cast<pgmap_t>(page_count);

      previous_page_offset = global.last_page_offset;
      global.last_page_offset += page_count;

      global.allocated_bytes += page_count * ia32::page_size;
      global.free_bytes      -= page_count * ia32::page_size;
    }

    //
    // Return the final address.
    // Note that we're not under lock here - we don't need it, because
    // everything neccessary has been done (bitmap + page allocation map
    // manipulation).
    //
    return global.base_address + previous_page_offset * ia32::page_size;
  }

  void free(void* address) noexcept
  {
    //
    // Our allocator always provides page-aligned memory.
    //
    hvpp_assert(ia32::byte_offset(address) == 0);

    const auto offset = static_cast<int>(ia32::bytes_to_pages(reinterpret_cast<uint8_t*>(address) - global.base_address));

    if (address == nullptr)
    {
      //
      // Return immediatelly if we're trying to free NULL.
      //
      // hvpp_assert(0);
      return;
    }

    if (size_t(offset) * ia32::page_size > global.available_size)
    {
      //
      // We don't own this memory.
      //
      hvpp_assert(0);
      return;
    }

    std::lock_guard _{ *global.lock };

    if (global.page_allocation_map[offset] == 0)
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
    const auto page_count = static_cast<int>(global.page_allocation_map[offset]);
    global.page_allocation_map[offset] = 0;

    //
    // Clear pages in the bitmap.
    //
    global.page_bitmap->clear(offset, page_count);

    global.allocated_bytes -= page_count * ia32::page_size;
    global.free_bytes      += page_count * ia32::page_size;
  }

  auto system_allocate(size_t size) noexcept -> void*
  {
    return detail::system_allocate(size);
  }

  void system_free(void* address) noexcept
  {
    detail::system_free(address);
  }

  auto allocated_bytes() noexcept -> size_t
  {
    return global.allocated_bytes;
  }

  auto free_bytes() noexcept -> size_t
  {
    return global.free_bytes;
  }

  auto allocator() noexcept -> const allocator_t&
  {
    return global.allocator[mp::cpu_index()];
  }

  void allocator(const allocator_t& new_allocator) noexcept
  {
    global.allocator[mp::cpu_index()] = new_allocator;
  }

  auto physical_memory_descriptor() noexcept -> const ia32::physical_memory_descriptor&
  {
    return *global.memory_descriptor;
  }

  auto mtrr() noexcept -> const ia32::mtrr&
  {
    return *global.memory_type_range_registers;
  }
}

namespace detail
{
  void* generic_allocate(size_t size) noexcept
  {
    return mm::global.allocator[mp::cpu_index()].allocate(size);
  }

  void generic_free(void* address) noexcept
  {
    reinterpret_cast<uint8_t*>(address) >= mm::global.base_address &&
    reinterpret_cast<uint8_t*>(address)  < mm::global.base_address + mm::global.available_size
      ? mm::free       (address)
      : mm::system_free(address);
  }
}

void* operator new  (size_t size)                                    { return detail::generic_allocate(size); }
void* operator new[](size_t size)                                    { return detail::generic_allocate(size); }
void* operator new  (size_t size, std::align_val_t)                  { return detail::generic_allocate(size); }
void* operator new[](size_t size, std::align_val_t)                  { return detail::generic_allocate(size); }

void operator delete  (void* address)                                { detail::generic_free(address); }
void operator delete[](void* address)                                { detail::generic_free(address); }
void operator delete[](void* address, std::size_t)                   { detail::generic_free(address); }
void operator delete  (void* address, std::size_t)                   { detail::generic_free(address); }
void operator delete  (void* address, std::align_val_t)              { detail::generic_free(address); }
void operator delete[](void* address, std::align_val_t)              { detail::generic_free(address); }
void operator delete[](void* address, std::size_t, std::align_val_t) { detail::generic_free(address); }
void operator delete  (void* address, std::size_t, std::align_val_t) { detail::generic_free(address); }
