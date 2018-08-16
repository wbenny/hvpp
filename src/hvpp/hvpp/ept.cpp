#include "ept.h"

#include "lib/assert.h"
#include "lib/bitmap.h"
#include "lib/mm.h"

namespace hvpp {

void ept_t::initialize() noexcept
{
  //
  // Initialize EPT's PML4. Each PML4 maps 512GB of memory. We would be fine
  // with just one PML4 in most scenarios, but we have to waste single page
  // on it anyway. Single page can handle 512 PML4s (their size is 8 bytes)
  // so just fill the whole page with 512 PML4s.
  //
  epml4_ = new epte_t[512];
  hvpp_assert(epml4_ != nullptr);
  memset(epml4_, 0, sizeof(epte_t) * 512);
  static_assert(sizeof(epte_t) * 512 == page_size);

  //
  // Get physical address of EPT's PML4.
  //
  pa_t empl4_pa = pa_t::from_va(epml4_);

  //
  // Initialize EPT pointer. It's not really JUST pointer, but Intel Manual calls
  // this structure as such.
  //
  eptptr_.flags = 0;
  eptptr_.memory_type = static_cast<uint64_t>(memory_manager::mtrr().type(empl4_pa));
  eptptr_.page_walk_length = ept_ptr_t::page_walk_length_4;
  eptptr_.page_frame_number = empl4_pa.pfn();
}

void ept_t::destroy() noexcept
{
  eptptr_.flags = 0;

  //
  // Note: this call also frees memory of epml4_ itself.
  //
  destroy(epml4_, page_table_level::pml4);
  epml4_ = nullptr;
}

ept_ptr_t ept_t::ept_pointer() const noexcept
{
  return eptptr_;
}

void ept_t::map_identity() noexcept
{
  //
  // This method will map the EPT in such way that they'll mirror host
  // physical memory to the guest. This means that physical memory 0x4000
  // in the guest will be physical memory 0x4000 in the host.
  //
  // We're going to try to achive that in an optimal way: we're not going
  // to map whole physical memory in 4kb granularity - instead we'll map
  // with 4kb granularity just those physical memory ranges, which actually
  // POINT to physical memory.
  //
  // Because things like DMA (Direct Memory Access) and IOMMU (I/O memory
  // management unit) are in this game too, some ranges of the physical memory
  // actually point to other devices. Because we don't care that much about
  // device's memory, we'll map it with 2MB granularity (to waste less space).
  //
  // Note that new memory ranges for devices can be created when PC is running;
  // for example if you plug-in USB device or any PnP device to the computer.
  // But because most of the time (not always!) these memory ranges reside in
  // lower 4GB of physical address space, we'll just cover the whole <4GB space
  // (which is not backed up by actual physical memory) with 2MB EPT pages.
  //
  // So, to sum it up:
  //   - Get physical memory descriptor (it will say us which ranges are actually
  //     backed up by physical memory).
  //   - Map these ranges with 4kb granularity.
  //   - Map rest of <4GB address space with 2MB EPT pages.
  //   - Because 2MB pages MUST be aligned to 2MB address boundary, there is
  //     a chance we still didn't cover the whole <4GB address space - so we
  //     will map these "holes" again with 4kb granularity.
  //

  //
  // Page frame number map for 0000'0000 - FFFF'FFFF range (first 4GB).
  // 1 bit == 1 page.
  //
  static constexpr uint64_t _4gb = 0x1'0000'0000;
  static constexpr uint64_t _2mb_pfn_count = 512;
  static constexpr uint64_t _4kb_pfn_count = 1;

  bitmap pfn_map(_4gb / page_size);
  int pfn;

  pfn = 0;
  for (auto range : memory_manager::physical_memory_descriptor())
  {
    for (auto pa : range)
    {
      map_4kb(pa, pa);

      if (pa < _4gb)
      {
        pfn_map.set(static_cast<uint32_t>(pa.pfn()));
      }
    }
  }

  //
  // Use 2MB pages whenever possible.
  //
  pfn = 0;
  while ((pfn = pfn_map.find_first_clear(pfn, _2mb_pfn_count)) != -1)
  {
    //
    // Check if the page is aligned to 2MB boundary.
    //
    if (!(pfn % _2mb_pfn_count))
    {
      auto pa = pa_t::from_pfn(pfn);
      map_2mb(pa, pa);

      pfn_map.set(pfn, _2mb_pfn_count);
    }
  }


  //
  // Use 4KB pages for the rest.
  //
  pfn = 0;
  while ((pfn = pfn_map.find_first_clear(pfn, _4kb_pfn_count)) != -1)
  {
    auto pa = pa_t::from_pfn(pfn);
    map_4kb(pa, pa);

    pfn_map.set(pfn);
  }

  hvpp_assert(pfn_map.all_set());
}

epte_t* ept_t::map(pa_t guest_pa, pa_t host_pa, epte_t::access_type access /* = epte_t::access_type::read_write_execute */, large_page large /* = large_page::none */) noexcept
{
  return map_pml4(guest_pa, host_pa, epml4_, access, large);
}

epte_t* ept_t::map_4kb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, large_page::none);
}

epte_t* ept_t::map_2mb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, large_page::pde_2mb);
}

epte_t* ept_t::map_1gb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, large_page::pdpte_1gb);
}

//
// Private
//

epte_t* ept_t::map_subtable(epte_t* table) noexcept
{
  //
  // Get or create next level of EPT table hierarchy.
  //
  // PML4 (top level)
  // -> PDPT
  //   -> PD
  //     -> PT
  //
  if (table->is_present())
  {
    return table->subtable();
  }

  auto subtable = new epte_t[512];
  hvpp_assert(subtable != nullptr);
  memset(subtable, 0, sizeof(epte_t) * 512);
  static_assert(sizeof(epte_t) * 512 == page_size);

  table->update(pa_t::from_va(subtable));
  return subtable;
}

epte_t* ept_t::map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4, epte_t::access_type access, large_page large) noexcept
{
  auto pml4e = &pml4[guest_pa.index(page_table_level::pml4)];
  auto pdpt = map_subtable(pml4e);

  return map_pdpt(guest_pa, host_pa, pdpt, access, large);
}

epte_t* ept_t::map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt, epte_t::access_type access, large_page large) noexcept
{
  auto pdpte = &pdpt[guest_pa.index(page_table_level::pdpt)];

  if (large == large_page::pdpte_1gb)
  {
    pdpte->update(host_pa, memory_manager::mtrr().type(guest_pa), true);
    return pdpte;
  }

  auto pd = map_subtable(pdpte);
  return map_pd(guest_pa, host_pa, pd, access, large);
}

epte_t* ept_t::map_pd(pa_t guest_pa, pa_t host_pa, epte_t* pd, epte_t::access_type access, large_page large) noexcept
{
  auto pde = &pd[guest_pa.index(page_table_level::pd)];

  if (large == large_page::pde_2mb)
  {
    pde->update(host_pa, memory_manager::mtrr().type(guest_pa), true);
    return pde;
  }

  auto pt = map_subtable(pde);
  return map_pt(guest_pa, host_pa, pt, access, large);
}

epte_t* ept_t::map_pt(pa_t guest_pa, pa_t host_pa, epte_t* pt, epte_t::access_type access, large_page large) noexcept
{
  auto page = &pt[guest_pa.index(page_table_level::pt)];

  (void)(large);
  hvpp_assert(large == large_page::none);
  {
    page->update(host_pa, memory_manager::mtrr().type(guest_pa), access);
    return page;
  }
}

void ept_t::destroy(epte_t* table, page_table_level ptl_type /* = page_table_level::pml4 */) noexcept
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
