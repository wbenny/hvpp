#include "driver.h"

#include "assert.h"
#include "mm.h"
#include "mp.h"
#include "log.h"

#include <cinttypes>

namespace driver::common
{
  void*  system_memory_ = nullptr;
  size_t system_memory_size_ = 0;

  driver_initialize_fn driver_initialize_;
  driver_destroy_fn    driver_destroy_;

  auto
  initialize(
    driver_initialize_fn driver_initialize,
    driver_destroy_fn driver_destroy
    ) noexcept -> error_code_t
  {
    hvpp_assert(system_memory_ == nullptr);
    hvpp_assert(system_memory_size_ == 0);

    //
    // Either both must be set or both must be nullptr,
    // nothing else.
    //
    hvpp_assert(!(!!driver_initialize ^ !!driver_destroy));

    driver_initialize_ = driver_initialize;
    driver_destroy_ = driver_destroy;

    //
    // Initialize logger and memory manager.
    //
    if (auto err = logger::initialize())
    {
      return err;
    }

    if (auto err = mm::initialize())
    {
      return err;
    }

    //
    // Print memory information to the debugger.
    //
    mm::mtrr().dump();
    mm::physical_memory_descriptor().dump();

    //
    // Estimate required memory size.
    // If hypervisor begins to run out of memory, required_memory_size
    // is the right variable to adjust.
    //
    // Default required memory size is 34MB per CPU.
    //
    const auto required_memory_size = (
      //
      // Estimated EPT size:
      // Make space for 2MB EPT entries for 512 GB of the physical
      // memory.  Each EPT entry has 8 bytes.
      // 512GB / 2MB * 8 = 256kb * 8 = 2MB per CPU.
      //
      ((512ull * 1024 * 1024 * 1024) / (2ull * 1024 * 1024) * 8)

      +

      //
      // Additional 32MB per CPU.
      //
      (32ull * 1024 * 1024)
      ) * mp::cpu_count();

    //
    // Round up to page boundary.
    //
    system_memory_size_ = ia32::round_to_pages(required_memory_size);

    hvpp_info("Number of processors: %u", mp::cpu_count());
    hvpp_info("Reserved memory:      %" PRIu64 " MB",
              system_memory_size_ / 1024 / 1024);

    //
    // Allocate memory.
    //
    system_memory_ = mm::system_allocate(required_memory_size);

    if (!system_memory_)
    {
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Assign allocated memory to the memory manager.
    //
    if (auto err = mm::assign(system_memory_, system_memory_size_))
    {
      return err;
    }

    return  driver_initialize_
      ? driver_initialize_()
      : error_code_t{};
  }

  void
  destroy() noexcept
  {
    //
    // Call driver's destroy() function, if provided.
    //
    if (driver_destroy_)
    {
      driver_destroy_();
    }

    //
    // Destroy memory manager and logger.
    //
    mm::destroy();
    logger::destroy();

    //
    // Return allocated memory back to the system.
    //
    if (system_memory_)
    {
      mm::system_free(system_memory_);
    }
  }
}
