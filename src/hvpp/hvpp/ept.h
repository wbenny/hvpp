#pragma once
#include "ia32/ept.h"
#include "ia32/memory.h"

#include "lib/error.h"

namespace hvpp {

using namespace ia32;

class ept_t final
{
  public:
    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;

    void map_identity(epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    epte_t* map    (pa_t guest_pa, pa_t host_pa,
                    epte_t::access_type access = epte_t::access_type::read_write_execute,
                    pml large = pml::pt) noexcept;

    epte_t* map_4kb(pa_t guest_pa, pa_t host_pa,
                    epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    epte_t* map_2mb(pa_t guest_pa, pa_t host_pa,
                    epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    epte_t* map_1gb(pa_t guest_pa, pa_t host_pa,
                    epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    void split_1gb_to_2mb(pa_t guest_pa, pa_t host_pa,
                          epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;
    void split_2mb_to_4kb(pa_t guest_pa, pa_t host_pa,
                          epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    void join_2mb_to_1gb(pa_t guest_pa, pa_t host_pa,
                          epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;
    void join_4kb_to_2mb(pa_t guest_pa, pa_t host_pa,
                          epte_t::access_type access = epte_t::access_type::read_write_execute) noexcept;

    epte_t*   ept_entry(pa_t guest_pa, pml level = pml::pt) noexcept;
    ept_ptr_t ept_pointer() const noexcept;

  private:
    template <
      typename ept_table_from_t,
      typename ept_table_to_t
    >
    void split(pa_t guest_pa, pa_t host_pa, epte_t::access_type access) noexcept;

    template <
      typename ept_table_from_t,
      typename ept_table_to_t
    >
    void join(pa_t guest_pa, pa_t host_pa, epte_t::access_type access) noexcept;

    epte_t* map_subtable(epte_t* table) noexcept;

    epte_t* map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4,
                     epte_t::access_type access, pml large) noexcept;
    epte_t* map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt,
                     epte_t::access_type access, pml large) noexcept;
    epte_t* map_pd  (pa_t guest_pa, pa_t host_pa, epte_t* pd,
                     epte_t::access_type access, pml large) noexcept;
    epte_t* map_pt  (pa_t guest_pa, pa_t host_pa, epte_t* pt,
                     epte_t::access_type access, pml large) noexcept;

    void unmap_table(epte_t* table, pml level = pml::pml4) noexcept;
    void unmap_entry(epte_t* entry, pml level) noexcept;

    ept_ptr_t eptptr_;
    epte_t*   epml4_;
};

}
