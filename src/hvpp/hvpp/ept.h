#pragma once
#include "ia32/ept.h"
#include "ia32/memory.h"

namespace hvpp {

using namespace ia32;

class ept_t
{
  public:
    enum class large_page
    {
      none,
      pde_2mb,
      pdpte_1gb,
    };

    void initialize() noexcept;
    void destroy() noexcept;

    ept_ptr_t ept_pointer() const noexcept;

    void map_identity() noexcept;
    epte_t* map(pa_t guest_pa, pa_t host_pa, epte_t::access_type access = epte_t::access_type::read_write_execute, large_page large = large_page::none) noexcept;

    epte_t* map_4kb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;
    epte_t* map_2mb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;
    epte_t* map_1gb(pa_t guest_pa, pa_t host_pa, epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

  private:
    epte_t* map_subtable(epte_t* table) noexcept;
    epte_t* map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4, epte_t::access_type access, large_page large) noexcept;
    epte_t* map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt, epte_t::access_type access, large_page large) noexcept;
    epte_t* map_pd  (pa_t guest_pa, pa_t host_pa, epte_t* pd,   epte_t::access_type access, large_page large) noexcept;
    epte_t* map_pt  (pa_t guest_pa, pa_t host_pa, epte_t* pt,   epte_t::access_type access, large_page large) noexcept;

    void destroy(epte_t* table, page_table_level ptl_type = page_table_level::pml4) noexcept;

    alignas(page_size) ept_ptr_t eptptr_;
                       epte_t*   epml4_;
};

}
