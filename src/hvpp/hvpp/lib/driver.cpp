#include "driver.h"

#include "assert.h"
#include "mm.h"
#include "mp.h"
#include "log.h"

#include <cinttypes>

namespace driver::common
{
  void*  system_memory = nullptr;
  size_t system_memory_size = 0;

  auto initialize() noexcept -> error_code_t
  {
    hvpp_assert(system_memory == nullptr);
    hvpp_assert(system_memory_size == 0);

    //
    // Initialize logger and memory manager.
    //
    if (auto err = logger::initialize())
    {
      return err;
    }

    if (auto err = memory_manager::initialize())
    {
      return err;
    }

    //
    // Print memory information to the debugger.
    //
    memory_manager::mtrr().dump();
    memory_manager::physical_memory_descriptor().dump();

    //
    // Estimate required memory size.
    // If hypervisor begins to run out of memory, required_memory_size
    // is the right variable to adjust.
    //
    // Default required memory size is 34MB per CPU.
    //
    auto required_memory_size = (
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
    system_memory_size = ia32::round_to_pages(required_memory_size);

    hvpp_info("Number of processors: %u", mp::cpu_count());
    hvpp_info("Reserved memory:      %" PRIu64 " MB",
              system_memory_size / 1024 / 1024);

    //
    // Allocate memory.
    //
    system_memory = memory_manager::system_allocate(required_memory_size);

    if (!system_memory)
    {
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Assign allocated memory to the memory manager.
    //
    if (auto err = memory_manager::assign(system_memory, system_memory_size))
    {
      return err;
    }

    return ::driver::initialize();
  }

  void destroy() noexcept
  {
    //
    // Call driver's destroy() function.
    //
    ::driver::destroy();

    //
    // Destroy memory manager and logger.
    //
    memory_manager::destroy();
    logger::destroy();

    //
    // Return allocated memory back to the system.
    //
    if (system_memory)
    {
      memory_manager::system_free(system_memory);
    }
  }
}
