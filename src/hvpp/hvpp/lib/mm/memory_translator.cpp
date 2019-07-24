#include "memory_translator.h"

#include "../mm.h"

namespace mm
{
  memory_translator::memory_translator() noexcept
  { }

  memory_translator::~memory_translator() noexcept
  { }

  va_t memory_translator::read(va_t va, cr3_t cr3, void* buffer, size_t size, bool ignore_errors /* = false */) noexcept
  {
    return read_write(va, cr3, buffer, size, false, ignore_errors);
  }

  va_t memory_translator::write(va_t va, cr3_t cr3, const void* buffer, size_t size, bool ignore_errors /* = false */) noexcept
  {
    return read_write(va, cr3, const_cast<void*>(buffer), size, true, ignore_errors);
  }

  pa_t memory_translator::va_to_pa(va_t va) noexcept
  {
    const auto pml4_base = paging_descriptor().pml4_base();
    const auto pml4 = &pml4_base[va.canonical().index(pml::pml4)];
    if (!pml4->present)
    {
      return {};
    }

    const auto pdpt_base = paging_descriptor().pdpt_base();
    const auto pdpt = &pdpt_base[va.canonical().index(pml::pdpt)];
    if (!pdpt->present)
    {
      return {};
    }

    if (pdpt->large_page)
    {
      return pa_t::from_pfn(pdpt->page_frame_number)
           + byte_offset(va.value(), pdpt_t{});
    }

    const auto pd_base = paging_descriptor().pd_base();
    const auto pd = &pd_base[va.canonical().index(pml::pd)];
    if (!pd->present)
    {
      return {};
    }

    if (pd->large_page)
    {
      return pa_t::from_pfn(pd->page_frame_number)
           + byte_offset(va.value(), pd_t{});
    }

    const auto pt_base = paging_descriptor().pt_base();
    const auto pt = &pt_base[va.canonical().index(pml::pt)];
    if (!pt->present)
    {
      return {};
    }

    return pa_t::from_pfn(pt->page_frame_number)
         + byte_offset(va.value());
  }

  pa_t memory_translator::va_to_pa(va_t va, cr3_t cr3) noexcept
  {
    pe_t pml4e;
    mapper_.read(pa_t::from_pfn(cr3.page_frame_number)
               + va.offset(pml::pml4) * sizeof(pe_t),
                 &pml4e, sizeof(pml4e));
    if (!pml4e.present)
    {
      return {};
    }

    pe_t pdpte;
    mapper_.read(pa_t::from_pfn(pml4e.page_frame_number)
               + va.offset(pml::pdpt) * sizeof(pe_t),
                 &pdpte, sizeof(pdpte));
    if (!pdpte.present)
    {
      return {};
    }

    if (pdpte.large_page)
    {
      return pa_t::from_pfn(pdpte.page_frame_number)
           + byte_offset(va.value(), pdpt_t{});
    }

    pe_t pde;
    mapper_.read(pa_t::from_pfn(pdpte.page_frame_number)
               + va.offset(pml::pd) * sizeof(pe_t),
                 &pde, sizeof(pde));
    if (!pde.present)
    {
      return {};
    }

    if (pde.large_page)
    {
      return pa_t::from_pfn(pde.page_frame_number)
           + byte_offset(va.value(), pd_t{});
    }

    pe_t pte;
    mapper_.read(pa_t::from_pfn(pde.page_frame_number)
               + va.offset(pml::pt) * sizeof(pe_t),
                 &pte, sizeof(pte));
    if (!pte.present)
    {
      return {};
    }

    return pa_t::from_pfn(pte.page_frame_number)
         + byte_offset(va.value());
  }

  va_t memory_translator::read_write(va_t va, cr3_t cr3, void* buffer, size_t size, bool write, bool ignore_errors) noexcept
  {
    auto byte_buffer = reinterpret_cast<uint8_t*>(buffer);

    //
    // Map each page of the physical memory to the reserved
    // system virtual address and then copy.
    //
    // If `ignore_errors == false', return address is the first
    // invalid virtual address (accessing it would trigger page
    // fault).
    //
    while (size != 0)
    {
      auto pa = va_to_pa(va, cr3);
      auto bytes_to_copy = page_size - byte_offset(pa.value());

      if (bytes_to_copy > size)
      {
        bytes_to_copy = size;
      }

      if (write)
      {
        if (pa)
        {
          mapper_.write(pa, byte_buffer, bytes_to_copy);
        }
        else if (ignore_errors)
        {
          /* NOTHING */;
        }
        else
        {
          return va;
        }
      }
      else
      {
        if (pa)
        {
          mapper_.read(pa, byte_buffer, bytes_to_copy);
        }
        else if (ignore_errors)
        {
          memset(byte_buffer, 0, bytes_to_copy);
        }
        else
        {
          return va;
        }
      }

      byte_buffer += bytes_to_copy;
      va          += bytes_to_copy;
      size        -= bytes_to_copy;
    }

    return {};
  }
}
