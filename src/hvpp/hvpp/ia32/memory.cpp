#include "memory.h"

namespace ia32 {

//////////////////////////////////////////////////////////////////////////
// va_t
//////////////////////////////////////////////////////////////////////////

pe_t* va_t::pt_entry(pml level /*= pml::pt*/) const noexcept
{
  const auto pml4e = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(read<cr3_t>().page_frame_number).va()
    )[offset(pml::pml4)];

  if (!pml4e->present || level == pml::pml4)
  {
    return pml4e;
  }

  const auto pdpte = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pml4e->page_frame_number).va()
    )[offset(pml::pdpt)];

  if (!pdpte->present || pdpte->large_page || level == pml::pdpt)
  {
    return pdpte;
  }

  const auto pde = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pdpte->page_frame_number).va()
    )[offset(pml::pd)];

  if (!pde->present || pde->large_page || level == pml::pd)
  {
    return pde;
  }

  const auto pt = &reinterpret_cast<pe_t*>(
    pa_t::from_pfn(pde->page_frame_number).va()
    )[offset(pml::pt)];

  return pt;
}

}
