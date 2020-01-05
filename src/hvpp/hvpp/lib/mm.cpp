#include "mm.h"

#include "assert.h"
#include "mp.h"
#include "../config.h"

namespace mm
{
  struct global_t
  {
    memory_allocator* allocator[HVPP_MAX_CPU];

    memory_allocator* system_allocator;
    memory_allocator* custom_allocator;

    object_t<paging_descriptor_t> paging_descriptor;
    object_t<physical_memory_descriptor_t> physical_memory_descriptor;
    object_t<mtrr_descriptor_t> mtrr_descriptor;
  };

  static global_t global;

  allocator_guard::allocator_guard() noexcept
    : allocator_guard(global.custom_allocator)
  {

  }

  allocator_guard::allocator_guard(memory_allocator* new_allocator) noexcept
    : previous_allocator_(allocator())
  {
    allocator(new_allocator);
  }

  allocator_guard::~allocator_guard() noexcept
  {
    allocator(previous_allocator_);
  }

  auto initialize() noexcept -> error_code_t
  {
    //
    // Initialize paging descriptor, physical memory descriptor
    // and MTRR descriptor.
    //
    global.paging_descriptor.initialize();
    global.physical_memory_descriptor.initialize();
    global.mtrr_descriptor.initialize();

    return {};
  }

  void destroy() noexcept
  {
    //
    // Destroy all objects.
    //
    global.mtrr_descriptor.destroy();
    global.physical_memory_descriptor.destroy();
    global.paging_descriptor.destroy();
  }

  auto system_allocator() noexcept -> memory_allocator*
  {
    return global.system_allocator;
  }

  void system_allocator(memory_allocator* new_allocator) noexcept
  {
    global.system_allocator = new_allocator;

    for (auto& allocator_item : global.allocator)
    {
      allocator_item = global.system_allocator;
    }
  }

  auto hypervisor_allocator() noexcept -> memory_allocator*
  {
    return global.custom_allocator;
  }

  void hypervisor_allocator(memory_allocator* new_allocator) noexcept
  {
    global.custom_allocator = new_allocator;
  }

  auto allocator() noexcept -> memory_allocator*
  {
    return global.allocator[mp::cpu_index()];
  }

  void allocator(memory_allocator* new_allocator) noexcept
  {
    hvpp_assert(new_allocator);
    global.allocator[mp::cpu_index()] = new_allocator;
  }

  auto paging_descriptor() noexcept -> const paging_descriptor_t&
  {
    return *global.paging_descriptor;
  }

  auto physical_memory_descriptor() noexcept -> const physical_memory_descriptor_t&
  {
    return *global.physical_memory_descriptor;
  }

  auto mtrr_descriptor() noexcept -> const mtrr_descriptor_t&
  {
    return *global.mtrr_descriptor;
  }
}

namespace detail
{
  void* generic_allocate(size_t size) noexcept
  {
    return mm::global.allocator[mp::cpu_index()]->allocate(size);
  }

  void* generic_allocate_aligned(size_t size, std::align_val_t alignment) noexcept
  {
    return mm::global.allocator[mp::cpu_index()]->allocate_aligned(size, static_cast<size_t>(alignment));
  }

  void generic_free(void* address) noexcept
  {
    return (mm::hypervisor_allocator() && mm::hypervisor_allocator()->contains(address))
      ? mm::hypervisor_allocator()->free(address)
      : mm::system_allocator()->free(address);
  }
}

void* operator new  (size_t size)                                    { return detail::generic_allocate        (size);            }
void* operator new[](size_t size)                                    { return detail::generic_allocate        (size);            }
void* operator new  (size_t size, std::align_val_t alignment)        { return detail::generic_allocate_aligned(size, alignment); }
void* operator new[](size_t size, std::align_val_t alignment)        { return detail::generic_allocate_aligned(size, alignment); }

void operator delete  (void* address)                                {        detail::generic_free(address);                     }
void operator delete[](void* address)                                {        detail::generic_free(address);                     }
void operator delete[](void* address, std::size_t)                   {        detail::generic_free(address);                     }
void operator delete  (void* address, std::size_t)                   {        detail::generic_free(address);                     }
void operator delete  (void* address, std::align_val_t)              {        detail::generic_free(address);                     }
void operator delete[](void* address, std::align_val_t)              {        detail::generic_free(address);                     }
void operator delete[](void* address, std::size_t, std::align_val_t) {        detail::generic_free(address);                     }
void operator delete  (void* address, std::size_t, std::align_val_t) {        detail::generic_free(address);                     }
