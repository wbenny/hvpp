#include "ept.h"
#include "lib/assert.h"
#include "lib/bitmap.h"

namespace hvpp {

void ept::initialize() noexcept
{
  mtrr_.initialize();

  epml4_ = new epte_t[512];
  pa_t empl4_pa = pa_t::from_va(epml4_);
  eptptr_.memory_type = static_cast<uint64_t>(mtrr_.type(empl4_pa));
  eptptr_.page_walk_length = EPT_PAGE_WALK_LENGTH_4;
  eptptr_.page_frame_number = empl4_pa.pfn();
}

void ept::destroy() noexcept
{
  eptptr_.flags = 0;

  destroy(epml4_, page_table_level::pml4);
  epml4_ = nullptr;
}

bool ept::is_initialized() const noexcept
{
  return
    emergency_pool_ != nullptr &&
    epml4_ != nullptr &&
    eptptr_.page_frame_number != 0;
}

void ept::map() noexcept
{
  //
  // Page map for 0000'0000 - FFFF'FFFF range (first 4GB)
  // 1 bit == 1 page (4GB / PAGE_SIZE / 8)
  //
  static constexpr uint32_t _4gb = 0xffffffff;
  static constexpr uint32_t _2mb_pfn_count = 512;
  static constexpr uint32_t _4kb_pfn_count = 1;

  bitmap pfn_map(8 * 128 * 1024);
  int pfn = 0;

  hv_log("getting memory descriptor...");

  physical_memory_descriptor mem_descriptor;
  mem_descriptor.initialize();

  hv_log("mapping 4kb range...");

  for (auto range : mem_descriptor)
  {
    for (auto pa : range)
    {
      map(pa, pa);

      if (pa < _4gb)
      {
        pfn_map.set(static_cast<uint32_t>(pa.pfn()));
      }
    }
  }

  hv_log("mapping 2mb range...");

  //
  // Use 2MB pages whenever possible.
  //
  pfn = 0;

  while ((pfn = pfn_map.find_first_clear(pfn, _2mb_pfn_count)) != -1)
  {
    if (!(pfn % _2mb_pfn_count))
    {
      auto pa = pa_t::from_pfn(pfn);
      map(pa, pa, page_table_level::pd);

      pfn_map.set(pfn, _2mb_pfn_count);
    }
  }

  hv_log("mapping rest with 4kb...");

  //
  // Use 4KB pages for the rest.
  //
  pfn = 0;

  while ((pfn = pfn_map.find_first_clear(pfn, _4kb_pfn_count)) != -1)
  {
    auto pa = pa_t::from_pfn(pfn);
    map(pa, pa);

    pfn_map.set(pfn);
  }

  hvpp_assert(pfn_map.all_set());

  hv_log("done!");
}

epte_t* ept::map(pa_t guest_pa, pa_t host_pa, page_table_level ptl_type /* = page_table_level::pt */) noexcept
{
  return map_pml4(guest_pa, host_pa, epml4_, ptl_type);
}

ept_ptr_t ept::ept_pointer() const noexcept
{
  return eptptr_;
}

//
// Private
//

epte_t* ept::map_subtable(epte_t* table) noexcept
{
  if (table->is_present())
  {
    return table->subtable();
  }

  auto subtable = new epte_t[512];
  table->update(pa_t::from_va(subtable));
  return subtable;
}

epte_t* ept::map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4, page_table_level ptl_type) noexcept
{
  auto pml4e = &pml4[guest_pa.index(page_table_level::pml4)];
  auto pdpt = map_subtable(pml4e);

  return map_pdpt(guest_pa, host_pa, pdpt, ptl_type);
}

epte_t* ept::map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt, page_table_level ptl_type) noexcept
{
  auto pdpte = &pdpt[guest_pa.index(page_table_level::pdpt)];

  if (ptl_type == page_table_level::pdpt)
  {
    pdpte->update(host_pa, mtrr_.type(guest_pa), true);
    return pdpte;
  }

  auto pd = map_subtable(pdpte);
  return map_pd(guest_pa, host_pa, pd, ptl_type);
}

epte_t* ept::map_pd(pa_t guest_pa, pa_t host_pa, epte_t* pd, page_table_level ptl_type) noexcept
{
  auto pde = &pd[guest_pa.index(page_table_level::pd)];

  if (ptl_type == page_table_level::pd)
  {
    pde->update(host_pa, mtrr_.type(guest_pa), true);
    return pde;
  }

  auto pt = map_subtable(pde);
  return map_pt(guest_pa, host_pa, pt, ptl_type);
}

epte_t* ept::map_pt(pa_t guest_pa, pa_t host_pa, epte_t* pt, page_table_level ptl_type) noexcept
{
  auto page = &pt[guest_pa.index(page_table_level::pt)];

  UNREFERENCED_PARAMETER(ptl_type);
  hvpp_assert(ptl_type == page_table_level::pt);
  {
    page->update(host_pa, mtrr_.type(guest_pa));
    return page;
  }
}

void ept::destroy(epte_t* table, page_table_level ptl_type /* = page_table_level::pml4 */) noexcept
{
  for (int i = 0; i < 512; ++i)
  {
    auto entry = &table[i];

    if (entry->is_present())
    {
      hvpp_assert(entry->page_frame_number != 0);

      auto subtable = entry->subtable();

      if (!entry->large_page)
      {
        switch (ptl_type)
        {
          case page_table_level::pml4:
          case page_table_level::pdpt:
            destroy(subtable, ptl_type - 1);
            break;

          case page_table_level::pd:
            delete subtable;
            break;

          case page_table_level::pt:
          default:
            hvpp_assert(0);
            break;
        }
      }
    }
  }

  delete[] table;
}

}
