#pragma once
#include "ia32/ept.h"
#include "ia32/memory.h"
#include "ia32/mtrr.h"

namespace hvpp {

using namespace ia32;

class ept
{
  public:
    void initialize() noexcept;
    void destroy() noexcept;

    bool is_initialized() const noexcept;

    void map() noexcept;
    epte_t* map(pa_t guest_pa, pa_t host_pa, page_table_level ptl_type = page_table_level::pt) noexcept;
    ept_ptr_t ept_pointer() const noexcept;

  private:
    epte_t* map_subtable(epte_t* table) noexcept;
    epte_t* map_pml4(pa_t guest_pa, pa_t host_pa, epte_t* pml4, page_table_level ptl_type) noexcept;
    epte_t* map_pdpt(pa_t guest_pa, pa_t host_pa, epte_t* pdpt, page_table_level ptl_type) noexcept;
    epte_t* map_pd  (pa_t guest_pa, pa_t host_pa, epte_t* pd,   page_table_level ptl_type) noexcept;
    epte_t* map_pt  (pa_t guest_pa, pa_t host_pa, epte_t* pt,   page_table_level ptl_type) noexcept;

    void destroy(epte_t* table, page_table_level ptl_type = page_table_level::pml4) noexcept;

    alignas(PAGE_SIZE) ept_ptr_t eptptr_;
                       epte_t*   epml4_;
                       mtrr      mtrr_;
                       void*     emergency_pool_;

};

}
