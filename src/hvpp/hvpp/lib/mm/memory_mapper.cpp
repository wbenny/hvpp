#include "memory_mapper.h"

namespace mm
{
  memory_mapper::memory_mapper() noexcept
  {
    //
    // Reserve 1 page of the virtual address space.
    // Note that the memory is NOT allocated, just reserved.
    //
    va_ = detail::mapper_allocate(page_size);

    //
    // Get page-table entry for the virtual address.
    //
    pte_ = va_t(va_).pt_entry();
  }

  memory_mapper::~memory_mapper() noexcept
  {
    //
    // Release the virtual address space.
    //
    detail::mapper_free(va_);
  }

  void* memory_mapper::map(pa_t pa) noexcept
  {
    //
    // Make this entry present & writable.
    //
    pte_->present = true;
    pte_->write = true;

    //
    // Do not flush this page from the TLB on CR3 switch.
    //
    pte_->global = true;

    //
    // Set the PFN of this PTE to the PFN of the
    // provided physical address.
    //
    pte_->page_frame_number = pa.pfn();

    //
    // Finally, invalidate the cache for the virtual address.
    //
    ia32_asm_inv_page(va_);

    return reinterpret_cast<uint8_t*>(va_) + byte_offset(pa.value());
  }

  void memory_mapper::unmap() noexcept
  {
    pte_->flags = 0;
  }

  void memory_mapper::read(pa_t pa, void* buffer, size_t size) noexcept
  {
    read_write(pa, buffer, size, false);
  }

  void memory_mapper::write(pa_t pa, const void* buffer, size_t size) noexcept
  {
    read_write(pa, const_cast<void*>(buffer), size, true);
  }

  void memory_mapper::read_write(pa_t pa, void* buffer, size_t size, bool write) noexcept
  {
    auto byte_buffer = reinterpret_cast<uint8_t*>(buffer);

    //
    // Map each page of the physical memory to the reserved
    // system virtual address and then copy.
    //
    while (size != 0)
    {
      auto va = map(pa);
      auto bytes_to_copy = page_size - byte_offset(va);

      if (bytes_to_copy > size)
      {
        bytes_to_copy = size;
      }

      if (write)
      {
        memcpy(va, byte_buffer, bytes_to_copy);
      }
      else
      {
        memcpy(byte_buffer, va, bytes_to_copy);
      }

      byte_buffer += bytes_to_copy;
      pa          += bytes_to_copy;
      size        -= bytes_to_copy;

      unmap();
    }
  }
}
