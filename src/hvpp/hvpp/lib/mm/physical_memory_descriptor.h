#pragma once
#include "hvpp/ia32/memory.h"
#include "../log.h"

namespace mm
{
  using namespace ia32;

  namespace detail
  {
    void check_physical_memory(physical_memory_range* range_list, int range_list_size, int& count) noexcept;
  }

  //
  // Class for receiving physical memory ranges which are backed up
  // by actual physical memory.
  //

  class physical_memory_descriptor_t
  {
    public:
      static constexpr auto max_range_count = 32;

      physical_memory_descriptor_t() noexcept { check_physical_memory(); }
      physical_memory_descriptor_t(const physical_memory_descriptor_t& other) noexcept = delete;
      physical_memory_descriptor_t(physical_memory_descriptor_t&& other) noexcept = delete;
      physical_memory_descriptor_t& operator=(const physical_memory_descriptor_t& other) noexcept = delete;
      physical_memory_descriptor_t& operator=(physical_memory_descriptor_t&& other) noexcept = delete;

      auto begin() const noexcept { return const_cast<const physical_memory_range*>(&range_[0]); }
      auto end()   const noexcept { return const_cast<const physical_memory_range*>(&range_[count_]); }
      auto size()  const noexcept { return static_cast<size_t>(count_); }

      auto total_physical_memory_size() const noexcept
      {
        return std::accumulate(begin(), end(), size_t{}, [](auto sum, auto next) {
          return sum + next.size();
        });
      }

      void dump() const noexcept
      {
        hvpp_info("Physical memory ranges (%i)", count_);

        for (int i = 0; i < count_; ++i)
        {
          hvpp_info(
            "  %3i)    [%016" PRIx64 " - %016" PRIx64 "] (%8u kb)", i,
            range_[i].begin().value(),
            range_[i].end().value(),
            range_[i].size() / 1024);
        }
      }

    private:
      void check_physical_memory() noexcept
      { detail::check_physical_memory(range_, max_range_count, count_); }

      physical_memory_range range_[max_range_count];
      int                   count_ = 0;
  };
}
