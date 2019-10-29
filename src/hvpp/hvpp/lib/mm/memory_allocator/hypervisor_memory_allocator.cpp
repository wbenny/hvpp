#include "hypervisor_memory_allocator.h"

#include "../../assert.h"
#include "../../../config.h"
#include "../../../ia32/memory.h"

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
  using namespace ia32;

  hypervisor_memory_allocator::hypervisor_memory_allocator() noexcept
    : base_address_{}
    , capacity_{}
    , page_bitmap_{}
    , page_bitmap_buffer_size_{}
    , page_allocation_map_{}
    , page_allocation_map_size_{}
    , last_page_offset_{}
    , allocated_bytes_{}
    , free_bytes_{}
    , lock_{}
  {

  }

  hypervisor_memory_allocator::~hypervisor_memory_allocator() noexcept
  {

  }

  auto hypervisor_memory_allocator::attach(void* address, size_t size) noexcept -> error_code_t
  {
    if (size < page_size * 3)
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
    if (byte_offset(address) != 0)
    {
      const auto lost_bytes = byte_offset(address);

      address = reinterpret_cast<uint8_t*>(page_align(address)) + page_size;

      //
      // Subtract amount of "lost" bytes due to alignment.
      //
      size -= lost_bytes;
    }

    //
    // Align size to the page boundary.
    //
    size = page_align(size);

    //
    // Check again.
    //
    if (size < page_size * 3)
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
    auto page_bitmap_buffer = reinterpret_cast<uint8_t*>(address);
    page_bitmap_buffer_size_ = static_cast<int>(round_to_pages(size / page_size / 8));
    memset(page_bitmap_buffer, 0, page_bitmap_buffer_size_);

    auto page_bitmap_size_in_bits = static_cast<int>(size / page_size);
    page_bitmap_ = pgbmp_t(page_bitmap_buffer, page_bitmap_size_in_bits);

    //
    // Construct the page allocation map.
    //
    page_allocation_map_ = reinterpret_cast<pgmap_t*>(page_bitmap_buffer + page_bitmap_buffer_size_);
    page_allocation_map_size_ = static_cast<int>(round_to_pages(size / page_size) * sizeof(pgmap_t));
    memset(page_allocation_map_, 0, page_allocation_map_size_);

    //
    // Compute available memory.
    //
    base_address_ = reinterpret_cast<uint8_t*>(address);
    capacity_ = size;

    //
    // Mark memory of page_bitmap and page_allocation_map as allocated.
    // The return value of these allocations should return the exact
    // address of page_bitmap_buffer and page_allocation_map.
    //
    auto page_bitmap_buffer_tmp  = allocate(page_bitmap_buffer_size_);
    auto page_allocation_map_tmp = allocate(page_allocation_map_size_);

    hvpp_assert(reinterpret_cast<uintptr_t>(page_bitmap_buffer)   == reinterpret_cast<uintptr_t>(page_bitmap_buffer_tmp));
    hvpp_assert(reinterpret_cast<uintptr_t>(page_allocation_map_) == reinterpret_cast<uintptr_t>(page_allocation_map_tmp));

    (void)(page_bitmap_buffer_tmp);
    (void)(page_allocation_map_tmp);

    //
    // Initialize memory pool with garbage.
    // This should help with debugging uninitialized variables
    // and class members.
    //
    const auto reserved_bytes = static_cast<int>(page_bitmap_buffer_size_ + page_allocation_map_size_);
    memset(base_address_ + reserved_bytes, 0xcc, size - reserved_bytes);

    //
    // Set initial values of allocated/free bytes.
    //
    allocated_bytes_ = 0;
    free_bytes_ = size;

    return {};
  }

  void hypervisor_memory_allocator::detach() noexcept
  {
    //
    // If no memory has been assigned - leave.
    //
    if (!base_address_)
    {
      return;
    }

    //
    // Note that this method doesn't acquire the lock and
    // assumes all allocations has been already freed.
    //

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
    free(page_bitmap_.buffer());
    free(page_allocation_map_);

    //
    // Checks for memory leaks.
    //
    hvpp_assert(page_bitmap_.all_clear());

    //
    // Checks for allocator corruption.
    //
    hvpp_assert(std::all_of(
      page_allocation_map_,
      page_allocation_map_ + page_allocation_map_size_ / sizeof(pgmap_t),
      [](auto page_count) { return page_count == 0; }));

    base_address_ = nullptr;
    capacity_ = 0;

    page_bitmap_buffer_size_ = 0;

    page_allocation_map_ = nullptr;
    page_allocation_map_size_ = 0;

    last_page_offset_ = 0;
    allocated_bytes_ = 0;
    free_bytes_ = 0;
  }

  auto hypervisor_memory_allocator::allocate(size_t size) noexcept -> void*
  {
    hvpp_assert(base_address_ != nullptr && capacity_ > 0);

    //
    // Return at least 1 page, even if someone required 0.
    //
    if (size == 0)
    {
      hvpp_assert(0);
      size = 1;
    }

    auto page_count = static_cast<int>(bytes_to_pages(size));

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
      std::lock_guard _{ lock_ };

      last_page_offset_ = page_bitmap_.find_first_clear(last_page_offset_, page_count);

      if (last_page_offset_ == -1)
      {
        last_page_offset_ = 0;
        last_page_offset_ = page_bitmap_.find_first_clear(last_page_offset_, page_count);

        if (last_page_offset_ == -1)
        {
          //
          // Not enough memory...
          //
          hvpp_assert(0);
          return nullptr;
        }
      }

      page_bitmap_.set(last_page_offset_, page_count);
      page_allocation_map_[last_page_offset_] = static_cast<pgmap_t>(page_count);

      previous_page_offset = last_page_offset_;
      last_page_offset_ += page_count;

      allocated_bytes_ += page_count * page_size;
      free_bytes_      -= page_count * page_size;
    }

    //
    // Return the final address.
    // Note that we're not under lock here - we don't need it, because
    // everything neccessary has been done (bitmap + page allocation map
    // manipulation).
    //
    return base_address_ + previous_page_offset * page_size;
  }

  auto hypervisor_memory_allocator::allocate_aligned(size_t size, size_t alignment) noexcept -> void*
  {
    //
    // Our allocator always returns page-aligned memory.
    // Therefore all alignments that are in interval (0, 4096]
    // AND are power of 2 are valid.
    //
    hvpp_assert(
      alignment > 0 &&
      alignment <= 4096 &&
      !(alignment & (alignment - 1)) // is power of 2
    );

    (void)(alignment);

    return allocate(size);
  }

  void hypervisor_memory_allocator::free(void* address) noexcept
  {
    //
    // Our allocator always provides page-aligned memory.
    //
    hvpp_assert(byte_offset(address) == 0);

    if (address == nullptr)
    {
      //
      // Return immediatelly if we're trying to free NULL.
      //
      // hvpp_assert(0);
      return;
    }

    const auto offset = static_cast<int>(bytes_to_pages(reinterpret_cast<uint8_t*>(address) - base_address_));

    if (size_t(offset) * page_size > capacity_)
    {
      //
      // We don't own this memory.
      //
      hvpp_assert(0);
      return;
    }

    std::lock_guard _{ lock_ };

    if (page_allocation_map_[offset] == 0)
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
    const auto page_count = static_cast<int>(page_allocation_map_[offset]);
    page_allocation_map_[offset] = 0;

    //
    // Clear pages in the bitmap.
    //
    page_bitmap_.clear(offset, page_count);

    allocated_bytes_ -= page_count * page_size;
    free_bytes_      += page_count * page_size;
  }

  bool hypervisor_memory_allocator::contains(void* address) noexcept
  {
    return
      reinterpret_cast<uint8_t*>(address) >= base_address_ &&
      reinterpret_cast<uint8_t*>(address)  < base_address_ + capacity_;
  }

  auto hypervisor_memory_allocator::allocated_bytes() noexcept -> size_t
  {
    return allocated_bytes_;
  }

  auto hypervisor_memory_allocator::free_bytes() noexcept -> size_t
  {
    return free_bytes_;
  }
}
