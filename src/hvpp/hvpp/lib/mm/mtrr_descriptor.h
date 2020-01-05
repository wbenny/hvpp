#pragma once
#include "hvpp/ia32/memory.h"
#include "hvpp/ia32/msr.h"
#include "hvpp/ia32/msr/mtrr.h"
#include "../typelist.h"
#include "../log.h"

#include <cstdint>
#include <cinttypes>

namespace mm
{
  using namespace ia32;

  struct mtrr_range
  {
    physical_memory_range range;
    memory_type           type;
  };

  class mtrr_descriptor_t
  {
    public:
      static constexpr auto fixed_count = (1 + 2 + 8) * 8;
      static constexpr auto max_variable_count = 255;

      mtrr_descriptor_t() noexcept { check_fixed(); check_variable(); }
      mtrr_descriptor_t(const mtrr_descriptor_t& other) noexcept = delete;
      mtrr_descriptor_t(mtrr_descriptor_t&& other) noexcept = delete;
      mtrr_descriptor_t& operator=(const mtrr_descriptor_t& other) noexcept = delete;
      mtrr_descriptor_t& operator=(mtrr_descriptor_t&& other) noexcept = delete;

      const mtrr_range* begin() const noexcept { return &mtrr_[0]; }
      const mtrr_range* end()   const noexcept { return &mtrr_[size()]; }
      size_t            size()  const noexcept { return fixed_count + variable_count_; }

      memory_type type(pa_t pa) const noexcept
      {
        //
        // If the MTRRs are not enabled (by setting the E flag in the
        // IA32_MTRR_DEF_TYPE MSR), then all memory accesses are of the
        // UC memory type.  If the MTRRs are enabled, then the memory
        // type used for a memory access is determined as follows:
        //
        // 1. If the physical address falls within the first 1 MByte of
        //    physical memory and fixed MTRRs are enabled, the processor
        //    uses the memory type stored for the appropriate fixed-range
        //    MTRR.
        //
        // 2. Otherwise, the processor attempts to match the physical
        //    address with a memory type set by the variable-range MTRRs:
        //    -  If one variable memory range matches, the processor uses
        //       the memory type stored in the IA32_MTRR_PHYSBASEn register
        //       for that range.
        //
        //    -  If two or more variable memory ranges match and the memory
        //       types are identical, then that memory type is used.
        //
        //    -  If two or more variable memory ranges match and one of the
        //       memory types is UC, the UC memory type is used.
        //
        //    -  If two or more variable memory ranges match and the memory
        //       types are WT and WB, the WT memory type is used.
        //
        //    -  For overlaps not defined by the above rules, processor
        //       behavior is undefined.
        //
        // 3. If no fixed or variable memory range matches, the processor uses
        //    the default memory type.
        //
        // (ref: Vol3A[11.11.4.1(MTRR Precedences)]
        //
        memory_type result = memory_type::invalid;

        for (auto mtrr_item : *this)
        {
          if (mtrr_item.range.contains(pa))
          {
            if (is_fixed(mtrr_item) || mtrr_item.type == memory_type::uncacheable)
            {
              result = mtrr_item.type;
              break;
            }

            if ( result == memory_type::write_back &&
                (result == memory_type::write_through ||
                 mtrr_item.type == memory_type::write_through))
            {
              result = memory_type::write_through;
            }
          }
        }

        if (result == memory_type::invalid)
        {
          result = default_memory_type_;
        }

        return result;
      }

      void dump() const noexcept
      {
        auto dump_range = []([[maybe_unused]] int i,
                             [[maybe_unused]] const mtrr_range& mtrr) noexcept
        {
          hvpp_info(
            "  %3i) %s [%016" PRIx64 " - %016" PRIx64 "] (%8u kb)", i,
            to_string(mtrr.type),
            mtrr.range.begin().value(),
            mtrr.range.end().value(),
            mtrr.range.size() / 1024);
        };

        hvpp_info("Fixed MTRR ranges (%i)", fixed_count);
        for (int i = 0; i < fixed_count; ++i)
        {
          dump_range(i, fixed_[i]);
        }

        hvpp_info("Variable MTRR ranges (%i)", variable_count_);
        for (int i = 0; i < variable_count_; ++i)
        {
          dump_range(i, variable_[i]);
        }
      }

    private:
      void check_fixed() noexcept
      {
        auto mtrr_default      = msr::read<msr::mtrr_def_type_t>();
        auto mtrr_capabilities = msr::read<msr::mtrr_capabilities_t>();

        default_memory_type_ = static_cast<memory_type>(mtrr_default.default_memory_type);

        if (mtrr_capabilities.fixed_range_supported && mtrr_default.fixed_range_mtrr_enable)
        {
          using mtrr_fix_64k_list_t  = type_list<msr::mtrr_fix_64k_00000_t>;
          using mtrr_fix_16k_list_t  = type_list<msr::mtrr_fix_16k_80000_t, msr::mtrr_fix_16k_a0000_t>;
          using mtrr_fix_4k_list_t   = type_list<msr::mtrr_fix_4k_c0000_t,  msr::mtrr_fix_4k_c8000_t,
                                                 msr::mtrr_fix_4k_d0000_t,  msr::mtrr_fix_4k_d8000_t,
                                                 msr::mtrr_fix_4k_e0000_t,  msr::mtrr_fix_4k_e8000_t,
                                                 msr::mtrr_fix_4k_f0000_t,  msr::mtrr_fix_4k_f8000_t>;
          using mtrr_fix_list_t      = type_list<mtrr_fix_64k_list_t, mtrr_fix_16k_list_t, mtrr_fix_4k_list_t>;

          for_each_type(mtrr_fix_list_t{}, [this](auto mtrr_fixed, int i) {
            using ia32_mtrr_t = decltype(mtrr_fixed);
            mtrr_fixed = msr::read<ia32_mtrr_t>();

            pa_t range = ia32_mtrr_t::mtrr_base;
            i *= 8;

            for (auto type : mtrr_fixed.type)
            {
              fixed_[i].range = physical_memory_range{ range, range + ia32_mtrr_t::mtrr_size };
              fixed_[i].type  = static_cast<memory_type>(type);

              range += ia32_mtrr_t::mtrr_size;
              i += 1;
            }
          });
        }
      }

      void check_variable() noexcept
      {
        auto mtrr_capabilities = msr::read<msr::mtrr_capabilities_t>();
        variable_count_ = mtrr_capabilities.variable_range_count;

        for (int i = 0; i < variable_count_; ++i)
        {
          auto mtrr_base = msr::read<msr::mtrr_physbase_t>(msr::mtrr_physbase_t::msr_id + i * 2);
          auto mtrr_mask = msr::read<msr::mtrr_physmask_t>(msr::mtrr_physmask_t::msr_id + i * 2);

          if (mtrr_mask.valid)
          {
            uint64_t size = 1ull << ia32_asm_bsf(mtrr_mask.page_frame_number);

            variable_[i].range = physical_memory_range(
              pa_t::from_pfn(mtrr_base.page_frame_number),
              pa_t::from_pfn(mtrr_base.page_frame_number + size));
            variable_[i].type  = static_cast<memory_type>(mtrr_base.type);
          }
        }
      }

      bool is_fixed(const mtrr_range& range) const noexcept
      {
        return (const mtrr_range*)&range < (const mtrr_range*)variable_;
      }

      bool is_variable(const mtrr_range& range) const noexcept
      {
        return (const mtrr_range*)&range >= (const mtrr_range*)variable_;
      }

      union
      {
        struct
        {
          mtrr_range fixed_[fixed_count];
          mtrr_range variable_[max_variable_count];
        };

        mtrr_range mtrr_[fixed_count + max_variable_count];
      };

      memory_type default_memory_type_ = memory_type::uncacheable;
      int variable_count_ = 0;
  };
}
