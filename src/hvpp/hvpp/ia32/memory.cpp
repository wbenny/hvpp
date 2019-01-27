#include "memory.h"

namespace ia32 {

//////////////////////////////////////////////////////////////////////////
// namespace detail
//////////////////////////////////////////////////////////////////////////

namespace detail
{
  uint64_t pa_from_va(void* va, cr3_t cr3) noexcept
  {
    return pa_t::from_pfn(
      va_t(va).pt_entry(cr3)->page_frame_number
      ).value() + byte_offset(va);
  }
}

//////////////////////////////////////////////////////////////////////////
// va_t
//////////////////////////////////////////////////////////////////////////

pe_t* va_t::pt_entry(cr3_t cr3 /*= read<cr3_t>()*/, pml level /*= pml::pt*/) const noexcept
{
  auto pml4e = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(cr3.page_frame_number).va()
    )[index(pml::pml4)];

  if (!pml4e->present || level == pml::pml4)
  {
    return pml4e;
  }

  auto pdpte = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pml4e->page_frame_number).va()
    )[index(pml::pdpt)];

  if (!pdpte->present || pdpte->large_page || level == pml::pdpt)
  {
    return pdpte;
  }

  auto pde = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pdpte->page_frame_number).va()
    )[index(pml::pd)];

  if (!pde->present || pde->large_page || level == pml::pd)
  {
    return pde;
  }

  auto pt = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pde->page_frame_number).va()
    )[index(pml::pt)];

  return pt;
}

//////////////////////////////////////////////////////////////////////////
// mapping_t
//////////////////////////////////////////////////////////////////////////

mapping_t::mapping_t() noexcept
{
  //
  // Reserve 1 page of the virtual address space.
  // Note that the memory is NOT allocated, just reserved.
  //
  va_ = detail::mapping_allocate(page_size);

  //
  // Get page-table entry for the virtual address.
  //
  pte_ = va_t(va_).pt_entry();
}

mapping_t::~mapping_t() noexcept
{
  //
  // Release the virtual address space.
  //
  detail::mapping_free(va_);
}

void* mapping_t::map(pa_t pa) noexcept
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

void mapping_t::unmap() noexcept
{
  pte_->flags = 0;
}

void mapping_t::read(pa_t pa, void* buffer, size_t size) noexcept
{
  read_write(pa, buffer, size, false);
}

void mapping_t::write(pa_t pa, const void* buffer, size_t size) noexcept
{
  read_write(pa, const_cast<void*>(buffer), size, true);
}

void mapping_t::read_write(pa_t pa, void* buffer, size_t size, bool write) noexcept
{
  uint8_t* byte_buffer = reinterpret_cast<uint8_t*>(buffer);

  //
  // Map each page of the physical memory to the reserved
  // virtual address and then copy.
  //
  while (size != 0)
  {
    void* va = map(pa);
    size_t bytes_to_copy = page_size - byte_offset(va);

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
    pa += bytes_to_copy;
    size -= bytes_to_copy;

    unmap();
  }
}

}
