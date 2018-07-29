#pragma once
#include "memory.h"
#include "msr.h"

#include <cstdint>
#include <ntddk.h>

namespace ia32 {

struct mtrr_range
{
  memory_range range;
  mtype        type;
};

class mtrr
{
  public:
    void initialize() noexcept
    {
      check_fixed();
      check_variable();
    }

    void destroy() noexcept
    {

    }

    const mtrr_range* begin() const noexcept { return &mtrr_[0]; }
    const mtrr_range* end()   const noexcept { return &mtrr_[11 * 8 + variable_count_]; }

    mtype type(pa_t pa) const noexcept
    {
      mtype result = mtype::invalid;

      for (auto mtrr_item : mtrr_)
      {
        if (mtrr_item.range.contains(pa))
        {
          if (is_fixed(mtrr_item) || mtrr_item.type == mtype::uncacheable)
          {
            result = mtrr_item.type;
            break;
          }

          if ((result == mtype::write_through || mtrr_item.type == mtype::write_through) &&
               result == mtype::write_back)
          {
            result = mtype::write_through;
          }
        }
      }

      if (result == mtype::invalid)
      {
        result = default_memory_type_;
      }

      return result;
    }

  private:
    void check_fixed() noexcept
    {
      auto mtrr_default      = msr::read<msr::mtrr_def_type>();
      auto mtrr_capabilities = msr::read<msr::mtrr_capabilities>();

      default_memory_type_ = static_cast<mtype>(mtrr_default.default_memory_type);

      if (mtrr_capabilities.fixed_range_supported && mtrr_default.fixed_range_mtrr_enable)
      {
        for_each_type(msr::mtrr_fix_list{}, [this](auto mtrr_fixed, int i) {
          using ia32_mtrr_t = decltype(mtrr_fixed);
          mtrr_fixed = msr::read<ia32_mtrr_t>();

          pa_t range = ia32_mtrr_t::mtrr_base;
          i *= 8;

          for (auto type : mtrr_fixed.type)
          {
            fixed_[i].range = memory_range(range, range + ia32_mtrr_t::mtrr_size);
            fixed_[i].type  = static_cast<mtype>(type);

            range += ia32_mtrr_t::mtrr_size;
            i += 1;
          }
        });
      }
    }

    void check_variable() noexcept
    {
      auto mtrr_capabilities = msr::read<msr::mtrr_capabilities>();
      variable_count_ = mtrr_capabilities.variable_range_count;

      for (int i = 0; i < variable_count_; ++i)
      {
        auto mtrr_base = msr::read<msr::mtrr_physbase>(msr::mtrr_physbase::msr_id + i * 2);
        auto mtrr_mask = msr::read<msr::mtrr_physmask>(msr::mtrr_physmask::msr_id + i * 2);

        if (mtrr_mask.valid)
        {
          uint64_t size = 1ull << ia32_asm_bsf(mtrr_mask.page_frame_number);

          variable_[i].range = memory_range(
            pa_t::from_pfn(mtrr_base.page_frame_number),
            pa_t::from_pfn(mtrr_base.page_frame_number + size));
          variable_[i].type  = static_cast<mtype>(mtrr_base.type);
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
        mtrr_range fixed_[11 * 8];
        mtrr_range variable_[255];
      };

      mtrr_range mtrr_[11 * 8 + 255];
    };

    mtype default_memory_type_ = mtype::uncacheable;
    uint8_t variable_count_ = 0;
};


}
