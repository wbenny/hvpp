#include "ept.h"

#include "lib/assert.h"
#include "lib/bitmap.h"
#include "lib/mm.h"

namespace hvpp {

auto ept_t::initialize() noexcept -> error_code_t
{
  //
  // Initialize EPT's PML4.  Each PML4 maps 512GB of memory. We would be fine
  // with just one PML4 in most scenarios, but we have to waste single page
  // on it anyway.  Single page can handle 512 PML4s (their size is 8 bytes)
  // so just fill the whole page with 512 PML4s.
  //
  static_assert(sizeof(epte_t) * 512 == page_size);

  epml4_ = new epte_t[512];
  hvpp_assert(epml4_ != nullptr);

  if (!epml4_)
  {
    return make_error_code_t(std::errc::not_enough_memory);
  }

  memset(epml4_, 0, sizeof(epte_t) * 512);

  //
  // Get physical address of EPT's PML4.
  //
  pa_t empl4_pa = pa_t::from_va(epml4_);

  //
  // Initialize EPT pointer.
  // It's not really JUST pointer, but Intel Manual calls it this way.
  //
  eptptr_.flags = 0;
  eptptr_.memory_type = static_cast<uint64_t>(memory_manager::mtrr().type(empl4_pa));
  eptptr_.page_walk_length = ept_ptr_t::page_walk_length_4;
  eptptr_.page_frame_number = empl4_pa.pfn();

  return error_code_t{};
}

void ept_t::destroy() noexcept
{
  eptptr_.flags = 0;

  if (epml4_)
  {
    unmap_table(epml4_);
    delete[] epml4_;

    epml4_ = nullptr;
  }
}

void ept_t::map_identity() noexcept
{
  //
  // This method will map the EPT in such way that they'll mirror
  // host physical memory to the guest.  This means that physical memory
  // at address 0x4000 in the guest will point to the physical memory
  // at address 0x4000 in the host.
  //
  // Only first 512 GB of physical memory is covered - hopefully that should
  // cover most cases - and 2MB pages are used.
  //
  // Usage of 2MB pages has following benefits:
  //   - Compared to 4kb pages, it requires less memory to cover the same
  //     address space.
  //   - CPU spends less time walking the paging hierarchy (one level less).
  //   - This function runs faster as it doesn't have to map 512 additional
  //     entries for each 2MB pages.
  //
  // Usage of 2MB pages has also following drawbacks:
  //   - Hooking of 2MB pages is inconvenient as we would get very frequent
  //     EPT violations.  If we want to do EPT hooking on smaller granularity
  //     (i.e. 4kb) we have to split desired 2MB page into 4kb pages.
  //   - Higher risk of MTRR conflict - see mtrr::type() method implementation -
  //     if two or more MTRRs are contained within single 2MB page and those
  //     MTRRs are of different types, the memory type of the whole 2MB page
  //     must be set to "least dangerous" option.  Worst case scenario is if
  //     UC (uncached) memory type collides with something else - the whole page
  //     must be then set to UC type.  This may result in slight performance
  //     loss.
  //

  static constexpr uint64_t _512gb = 512ull * 1024
                                            * 1024
                                            * 1024;

  for (pa_t pa = 0; pa < _512gb; pa += ept_pd_t::size)
  {
    map_2mb(pa, pa);
  }
}

epte_t* ept_t::map(pa_t guest_pa, pa_t host_pa,
                   epte_t::access_type access /* = epte_t::access_type::read_write_execute */,
                   pml large /* = pml::pt */) noexcept
{
  //
  // Map provided guest physical address to provided host physical address.
  // Set provided access to the guest physical address.
  // The map is created using paging structure specified in "large" parameter.
  // The range of mapped memory is derived from the size of the paging
  // structure.
  //
  return map_pml4(guest_pa, host_pa, epml4_, access, large);
}

epte_t* ept_t::map_4kb(pa_t guest_pa, pa_t host_pa,
                       epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, pml::pt);
}

epte_t* ept_t::map_2mb(pa_t guest_pa, pa_t host_pa,
                       epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, pml::pd);
}

epte_t* ept_t::map_1gb(pa_t guest_pa, pa_t host_pa,
                       epte_t::access_type access /* = epte_t::access_type::read_write_execute */) noexcept
{
  return map(guest_pa, host_pa, access, pml::pdpt);
}

void ept_t::split_1gb_to_2mb(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Split
  //    PDPT entry (1, large, 1GB)
  //          into
  //    PD entries (512, large, 2MB).
  //
  split<ept_pdpt_t, ept_pd_t>(guest_pa, host_pa);
}

void ept_t::split_2mb_to_4kb(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Split
  //    PD entry (1, large, 2MB)
  //          into
  //    PT entries (512, 4kb).
  //
  split<ept_pd_t, ept_pt_t>(guest_pa, host_pa);
}

void ept_t::join_2mb_to_1gb(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Join (merge)
  //    PD entries (512, large, 2MB)
  //          into
  //    PDPT entry (1, large, 1GB).
  //
  join<ept_pd_t, ept_pdpt_t>(guest_pa, host_pa);
}

void ept_t::join_4kb_to_2mb(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Join (merge)
  //    PT entries (512, 4kb)
  //          into
  //    PD entry (1, large, 2MB).
  //
  join<ept_pt_t, ept_pd_t>(guest_pa, host_pa);
}

ept_ptr_t ept_t::ept_pointer() const noexcept
{
  return eptptr_;
}

//
// Private
//

template <
  typename ept_table_from_t,
  typename ept_table_to_t
>
void ept_t::split(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Sanity compile-time checks - allow to use only EPT descriptors.
  // See ia32/ept.h.
  //
  static_assert(std::is_base_of_v<ept_descriptor_tag,
                                  ept_table_from_t::descriptor_tag>,
                "Wrong ept_table_from_t type");


  static_assert(std::is_base_of_v<ept_descriptor_tag,
                                  ept_table_to_t::descriptor_tag>,
                "Wrong ept_table_to_t type");

  //
  // Splitting pages (breaking large page into smaller ones) is possible
  // only down accross one level (i.e. PDPT -> PD, PD -> PT).  If you
  // desire to split PDPT into PTs (2 levels apart), you have to split
  // the PDPT first and then split resulting PDs again into PTs.
  //
  static_assert(ept_table_from_t::level - 1 == ept_table_to_t::level,
                "Cannot split page-tables accros multiple levels");

  //
  // PML4 is not splittable (doesn't have the "large_page" flag).
  // Although this might change in the future CPUs.
  //
  static_assert(ept_table_from_t::level != pml::pml4,
                "Cannot split PML4 into PDPTs");

  //
  // Fetch the EPT entry for the provided guest physical address.
  // The returned EPT entry is fetched at the "ept_table_from_t::level",
  // this means that if we're splitting from PD to PTs, we've fetched PD entry.
  //
  auto entry = ept_entry(guest_pa, ept_table_from_t::level);

  //
  // Make sure that the fetched entry is indeed large.
  // We can't split non-large pages - they already are splitted.
  //
  hvpp_assert(entry->large_page);

  //
  // Unmap the entry, i.e. make it non-present and reset the PFN.
  //
  unmap_entry(entry, ept_table_from_t::level);

  //
  // Map the unmapped physical memory range again, this time with smaller
  // EPT entries.
  //
  // If we're splitting 2MB page into 4kb pages, we're mapping range
  // [ guest_pa, guest_pa + 2MB ].
  //
  // Note: Each paging structure on Intel architectures always contain 512
  //       entries (one PDPT can contain 512 PDs, one PD can contain 512 PTs,
  //       one PT can contain 512 pages).  This is convenient because at the
  //       same time, each EPT structure has 8 bytes - therefore those 512
  //       entries can always fit into single 4kb page.
  //
  //       This means that this for-loop always exetues 512-times, no matter
  //       what the ept_table_from_t is.
  //
  for (uint64_t i = 0; i < ept_table_from_t::count; i++)
  {
    map(guest_pa + (i * ept_table_to_t::size), // offset = iteration * page_size
        host_pa  + (i * ept_table_to_t::size), // offset = iteration * page_size
        epte_t::access_type::read_write_execute,
        ept_table_to_t::level);
  }
}

template <
  typename ept_table_from_t,
  typename ept_table_to_t
>
void ept_t::join(pa_t guest_pa, pa_t host_pa) noexcept
{
  //
  // Sanity compile-time checks - allow to use only EPT descriptors.
  // See ia32/ept.h.
  //
  static_assert(std::is_base_of_v<ept_descriptor_tag,
                                  ept_table_from_t::descriptor_tag>,
                "Wrong ept_table_from_t type");


  static_assert(std::is_base_of_v<ept_descriptor_tag,
                                  ept_table_to_t::descriptor_tag>,
                "Wrong ept_table_to_t type");

  //
  // Joining pages (merging pages into one large page) is possible
  // only up accross one level (i.e. PD -> PDPT, PT -> PD).  If you
  // desire to join PTs into PDPT (2 levels apart), you have to join
  // each PT into PDs first and then join resulting PDs again into PDPT.
  //
  static_assert(ept_table_from_t::level + 1 == ept_table_to_t::level,
                "Cannot join page-tables accros multiple levels");

  //
  // PDPTs are not joinable into PML4 (PML4 doesn't have the "large_page" flag).
  // Although this might change in the future CPUs.
  //
  static_assert(ept_table_from_t::level != pml::pdpt,
                "Cannot join PDPTs to PML4");

  //
  // Make sure the guest physical address is aligned with respect to the
  // target EPT structure; i.e. if we're joining PTs into PD, the guest_pa
  // must be aligned to 2MB boundary.  If we're joining PDs into PDPT,
  // the guest_pa must be aligned to 1GB boundary.
  //
  hvpp_assert((guest_pa & ept_table_to_t::mask) == guest_pa);
  guest_pa = page_align(guest_pa.value(), ept_table_to_t{});

  //
  // Fetch the EPT entry for the provided guest physical address.
  // The returned EPT entry is fetched at the "ept_table_to_t::level",
  // this means that if we're joining PTs into PD, we've fetched PD entry.
  //
  auto entry = ept_entry(guest_pa, ept_table_to_t::level);

  //
  // Make sure that the fetched entry is not large.
  // We can't join large pages.
  //
  if (entry->large_page)
  {
    hvpp_assert(0);
    return;
  }

  //
  // Unmap the entry, i.e. make it non-present and reset the PFN.
  // This will also deallocate entries in the subtable (e.g. if entry is PD,
  // the PT it points to (entry->page_frame_number) will get automatically
  // deallocated).
  //
  unmap_entry(entry, ept_table_to_t::level);

  //
  // Map the unmapped physical memory range again, this time with single
  // large EPT entry.
  //
  map(guest_pa,
      host_pa,
      epte_t::access_type::read_write_execute,
      ept_table_to_t::level);
}

epte_t* ept_t::ept_entry(pa_t guest_pa, pml level /* = pml::pt */) noexcept
{
  //
  // Get EPT entry at desired level for provided guest physical address.
  // Start at PML4 and traverse down the paging hierarchy.
  // Returns nullptr for unmapped (non-present) physical addresses.
  //
  auto pml4e = &epml4_[guest_pa.index(pml::pml4)];
  auto pdpte = pml4e->is_present()
    ? &pml4e->subtable()[guest_pa.index(pml::pdpt)]
    : nullptr;

  if (!pdpte || pdpte->large_page || level == pml::pdpt)
  {
    return pdpte;
  }

  auto pde = pdpte->is_present()
    ? &pdpte->subtable()[guest_pa.index(pml::pd)]
    : nullptr;

  if (!pde || pde->large_page || level == pml::pd)
  {
    return pde;
  }

  auto pte = pde->is_present()
    ? &pde->subtable()[guest_pa.index(pml::pt)]
    : nullptr;

  return pte;
}

epte_t* ept_t::map_subtable(epte_t* table) noexcept
{
  //
  // Get or create next level of EPT table hierarchy.
  // PML4 -> PDPT -> PD -> PT
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

epte_t* ept_t::map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4,
                        epte_t::access_type access, pml large) noexcept
{
  auto pml4e = &pml4[guest_pa.index(pml::pml4)];
  auto pdpt = map_subtable(pml4e);

  return map_pdpt(guest_pa, host_pa, pdpt, access, large);
}

epte_t* ept_t::map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt,
                        epte_t::access_type access, pml large) noexcept
{
  auto pdpte = &pdpt[guest_pa.index(pml::pdpt)];

  if (large == pml::pdpt)
  {
    pdpte->update(host_pa, memory_manager::mtrr().type(guest_pa), true);
    return pdpte;
  }

  auto pd = map_subtable(pdpte);
  return map_pd(guest_pa, host_pa, pd, access, large);
}

epte_t* ept_t::map_pd(pa_t guest_pa, pa_t host_pa, epte_t* pd,
                      epte_t::access_type access, pml large) noexcept
{
  auto pde = &pd[guest_pa.index(pml::pd)];

  if (large == pml::pd)
  {
    pde->update(host_pa, memory_manager::mtrr().type(guest_pa), true);
    return pde;
  }

  auto pt = map_subtable(pde);
  return map_pt(guest_pa, host_pa, pt, access, large);
}

epte_t* ept_t::map_pt(pa_t guest_pa, pa_t host_pa, epte_t* pt,
                      epte_t::access_type access, pml large) noexcept
{
  auto pte = &pt[guest_pa.index(pml::pt)];

  (void)(large);
  hvpp_assert(large == pml::pt);
  {
    pte->update(host_pa, memory_manager::mtrr().type(guest_pa), access);
    return pte;
  }
}

void ept_t::unmap_table(epte_t* table, pml level /* = pml::pml4 */) noexcept
{
  //
  // Table must be present and it cannot be large page.
  //
  hvpp_assert(table && table->is_present() && !table->large_page);

  //
  // PTs already point to real physical addresses - we can't unmap them.
  //
  hvpp_assert(level != pml::pt);

  //
  // Unmap each of 512 entries in the table.
  //
  for (int i = 0; i < 512; ++i)
  {
    auto entry = &table[i];
    unmap_entry(entry, level);
  }
}

void ept_t::unmap_entry(epte_t* entry, pml level) noexcept
{
  hvpp_assert(entry);

  if (!entry->is_present())
  {
    //
    // Don't do anything if entry is already unmapped.
    //
    return;
  }

  //
  // PFN cannot be 0 unless the page is large.
  //
  // For example:
  //   2MB large PD entry which covers first 2MB of the RAM will have PFN = 0.
  //   But non-large PD entry which points to the page-table can't have PFN = 0.
  //
  hvpp_assert(entry->page_frame_number != 0 || entry->large_page);

  if (!entry->large_page)
  {
    //
    // Fetch subtable. Only non-large pages have subtables.
    //
    auto subtable = entry->subtable();

    //
    // Unmap and/or deallocate the subtable based on current page map level.
    //
    switch (level)
    {
      case pml::pml4:
      case pml::pdpt:
        if (!subtable->large_page)
        {
          unmap_table(subtable, level - 1);
        }
        delete[] subtable;
        break;

        case pml::pd:
          delete[] subtable;
          break;

        case pml::pt:
        default:
          hvpp_assert(0);
          break;
        }
  }

  //
  // Unmap entry and make it not present.
  //
  entry->clear();
}

}
