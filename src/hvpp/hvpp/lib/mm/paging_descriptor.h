#pragma once
#include "hvpp/ia32/memory.h"
#include "../log.h"

#include <cstdint>
#include <cinttypes>

namespace mm
{
  using namespace ia32;

  namespace detail
  {
    void check_paging(uint64_t* pml4_base, uint64_t* pdpt_base, uint64_t* pd_base, uint64_t* pt_base) noexcept;
    void check_system_cr3(uint64_t* system_cr3) noexcept;
  }

  class paging_descriptor_t
  {
    public:
      paging_descriptor_t() noexcept { check_paging(); check_system_cr3(); }
      paging_descriptor_t(const paging_descriptor_t& other) noexcept = delete;
      paging_descriptor_t(paging_descriptor_t&& other) noexcept = delete;
      paging_descriptor_t& operator=(const paging_descriptor_t& other) noexcept = delete;
      paging_descriptor_t& operator=(paging_descriptor_t&& other) noexcept = delete;

      auto pml4_base()  const noexcept -> pe_t* { return pml4_base_;  }
      auto pdpt_base()  const noexcept -> pe_t* { return pdpt_base_;  }
      auto pd_base()    const noexcept -> pe_t* { return pd_base_;    }
      auto pt_base()    const noexcept -> pe_t* { return pt_base_;    }

      auto system_cr3() const noexcept -> cr3_t { return system_cr3_; }

      void dump() const noexcept
      {
        hvpp_info("Paging information");
        hvpp_info("      PML4 base             - %016" PRIx64, pml4_base_);
        hvpp_info("      PDPT base             - %016" PRIx64, pdpt_base_);
        hvpp_info("        PD base             - %016" PRIx64, pd_base_);
        hvpp_info("        PT base             - %016" PRIx64, pt_base_);
        hvpp_info("");
        hvpp_info("     System CR3             - %016" PRIx64, system_cr3_.flags);
      }

    private:
      void check_paging() noexcept
      { detail::check_paging(
          reinterpret_cast<uint64_t*>(&pml4_base_),
          reinterpret_cast<uint64_t*>(&pdpt_base_),
          reinterpret_cast<uint64_t*>(&pd_base_),
          reinterpret_cast<uint64_t*>(&pt_base_)
        ); }

      void check_system_cr3() noexcept
      { detail::check_system_cr3(&system_cr3_.flags); }

      pe_t* pml4_base_; // PXE_BASE
      pe_t* pdpt_base_; // PPE_BASE
      pe_t* pd_base_;   // PDE_BASE
      pe_t* pt_base_;   // PTE_BASE

      cr3_t system_cr3_;
  };
}
