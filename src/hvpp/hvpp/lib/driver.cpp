#include "driver.h"

#include "assert.h"
#include "mm.h"
#include "mp.h"
#include "object.h"
#include "log.h"

#include <cinttypes>

namespace driver::common
{
  static driver_initialize_fn driver_initialize_;
  static driver_destroy_fn    driver_destroy_;

  static object_t<mm::system_memory_allocator> system_memory_allocator_object_;
  static object_t<mm::hypervisor_memory_allocator> hypervisor_memory_allocator_object_;

  static bool   has_default_hypervisor_allocator_ = false;
  static void*  hypervisor_allocator_base_address_ = nullptr;
  static size_t hypervisor_allocator_capacity_ = 0;
         size_t hypervisor_allocator_capacity__ = 0;  // read from registry

  auto
  initialize(
    driver_initialize_fn driver_initialize,
    driver_destroy_fn driver_destroy
    ) noexcept -> error_code_t
  {
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
    mm::mtrr_descriptor().dump();
    mm::physical_memory_descriptor().dump();
    mm::paging_descriptor().dump();

    //
    // Initialize default system allocator.
    //
    if (auto err = system_allocator_default_initialize())
    {
      return err;
    }

    //
    // If driver doesn't have initialize() function, we're finished.
    //
    if (!driver_initialize_)
    {
      return {};
    }

    //
    // ...otherwise, call the provided initialize() function.
    //
    if (auto err = driver_initialize_())
    {
      return err;
    }

    //
    // Check if hypervisor allocator has been set.
    //
    if (mm::hypervisor_allocator())
    {
      return {};
    }

    //
    // ...if not, create default hypervisor allocator.
    //
    if (auto err = hypervisor_allocator_default_initialize())
    {
      return err;
    }

    return {};
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
    // Destroy default hypervisor allocator (if constructed).
    //
    if (has_default_hypervisor_allocator_)
    {
      hypervisor_allocator_default_destroy();
    }

    //
    // At last, destroy default system allocator.
    //
    system_allocator_default_destroy();
  }

  auto system_allocator_default_initialize() noexcept -> error_code_t
  {
    //
    // Construct and assign system allocator object.
    //
    system_memory_allocator_object_.initialize();
    mm::system_allocator(&*system_memory_allocator_object_);

    return {};
  }

  void system_allocator_default_destroy() noexcept
  {
    //
    // Unassign system allocator and destroy the object.
    //
    mm::system_allocator(nullptr);
    system_memory_allocator_object_.destroy();
  }

  auto hypervisor_allocator_default_initialize() noexcept -> error_code_t
  {
    hvpp_assert(hypervisor_allocator_base_address_ == nullptr);
    hvpp_assert(hypervisor_allocator_capacity_ == 0);

    //
    // Construct hypervisor allocator object.
    //
    hypervisor_memory_allocator_object_.initialize();

    hypervisor_allocator_capacity_ = hypervisor_allocator_recommended_capacity();

    hvpp_info("Number of processors: %u", mp::cpu_count());
    hvpp_info("Reserved memory:      %" PRIu64 " MB",
              hypervisor_allocator_capacity_ / 1024 / 1024);

    //
    // Allocate memory.
    //
    hypervisor_allocator_base_address_ = mm::system_allocator()->allocate(hypervisor_allocator_capacity_);

    if (!hypervisor_allocator_base_address_)
    {
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Attach allocated memory.
    //
    if (auto err = hypervisor_memory_allocator_object_->attach(hypervisor_allocator_base_address_, hypervisor_allocator_capacity_))
    {
      return err;
    }

    //
    // Assign allocator.
    //
    mm::hypervisor_allocator(&*hypervisor_memory_allocator_object_);

    has_default_hypervisor_allocator_ = true;

    return {};
  }

  void hypervisor_allocator_default_destroy() noexcept
  {
    if (!hypervisor_allocator_base_address_)
    {
      return;
    }

    if (!mm::hypervisor_allocator())
    {
      return;
    }

    //
    // Unassign allocator.
    //
    mm::hypervisor_allocator(nullptr);

    //
    // Detach allocated memory.
    //
    hypervisor_memory_allocator_object_->detach();

    //
    // Destroy object.
    //
    hypervisor_memory_allocator_object_.destroy();

    //
    // Return allocated memory back to the system.
    //
    mm::system_allocator()->free(hypervisor_allocator_base_address_);

    hypervisor_allocator_base_address_ = nullptr;
    hypervisor_allocator_capacity_ = 0;
  }

  auto hypervisor_allocator_recommended_capacity() noexcept -> size_t
  {
    //
    // If allocator capacity was set in the registry, prefer that value.
    //

    if (hypervisor_allocator_capacity__ > 0)
    {
      return hypervisor_allocator_capacity__;
    }

    //
    // Estimate required memory size.
    // If hypervisor begins to run out of memory, required_memory_size
    // is the right variable to adjust.
    //
    // Default required memory size is 34MB per CPU.
    //
    const auto recommended_memory_size = (
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
    return ia32::round_to_pages(recommended_memory_size);
  }
}
