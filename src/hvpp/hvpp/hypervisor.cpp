#include "hypervisor.h"
#include "config.h"

#include "ia32/cpuid/cpuid_eax_01.h"
#include "lib/assert.h"
#include "lib/driver.h"
#include "lib/log.h"
#include "lib/mm.h"
#include "lib/mp.h"

namespace hvpp::hypervisor
{
  namespace detail
  {
    static
    bool
    check_cpu_features(
      void
      ) noexcept
    {
      cpuid_eax_01 cpuid_info;
      ia32_asm_cpuid(cpuid_info.cpu_info, 1);
      if (!cpuid_info.feature_information_ecx.virtual_machine_extensions)
      {
        return false;
      }

      const auto cr4 = read<cr4_t>();
      if (cr4.vmx_enable)
      {
        return false;
      }

      const auto vmx_basic = msr::read<msr::vmx_basic_t>();
      if (
          vmx_basic.vmcs_size_in_bytes > page_size ||
          vmx_basic.memory_type != uint64_t(memory_type::write_back) ||
         !vmx_basic.true_controls
        )
      {
        return false;
      }

      const auto vmx_ept_vpid_cap = msr::read<msr::vmx_ept_vpid_cap_t>();
      if (
        !vmx_ept_vpid_cap.page_walk_length_4      ||
        !vmx_ept_vpid_cap.memory_type_write_back  ||
        !vmx_ept_vpid_cap.invept                  ||
        !vmx_ept_vpid_cap.invept_all_contexts     ||
        !vmx_ept_vpid_cap.execute_only_pages      ||
        !vmx_ept_vpid_cap.pde_2mb_pages
        )
      {
        return false;
      }

      return true;
    }
  }

  struct global_t
  {
    vcpu_t* vcpu_list;
    bool    running;
  };

  static global_t global;

  auto start(vmexit_handler& handler) noexcept -> error_code_t
  {
    //
    // If hypervisor is already running,
    // don't do anything.
    //
    hvpp_assert(!global.running);
    if (global.running)
    {
      return make_error_code_t(std::errc::operation_not_permitted);
    }

    //
    // Create array of VCPUs.
    //
    // Note that since:
    //   - vcpu_t is not default-constructible
    //   - operator new[] doesn't support constructing objects
    //     with parameters
    // ... we have to construct this array "placement new".
    //
    hvpp_assert(global.vcpu_list == nullptr);

    global.vcpu_list = reinterpret_cast<vcpu_t*>(operator new(sizeof(vcpu_t) * mp::cpu_count()));
    if (!global.vcpu_list)
    {
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Construct each vcpu_t object as `vcpu_t(handler)'.
    //
    std::for_each_n(global.vcpu_list, mp::cpu_count(),
      [&](vcpu_t& vp) {
        ::new (std::addressof(vp)) vcpu_t(handler);
      });

    //
    // Check if hypervisor-allocator has been set.
    //
    // If the driver is calling `hypervisor::start()' from
    // the `driver::initialize()' method without manually
    // setting the allocator, then the allocator will be empty.
    //
    if (!mm::hypervisor_allocator())
    {
      //
      // Hypervisor-allocator has not been set - create default one.
      //
      // Note:
      //   Default hypervisor allocator is automatically destroyed
      //   in the `driver::common::destroy()' function.
      //
      if (auto err = driver::common::hypervisor_allocator_default_initialize())
      {
        return err;
      }
    }

    //
    // Check that CPU supports all required features to
    // run this hypervisor.
    //
    // Note:
    //   This check is performed only on current CPU
    //   and assumes all CPUs are symmetrical.
    //
    if (!detail::check_cpu_features())
    {
      return make_error_code_t(std::errc::not_supported);
    }

    //
    // Start virtualization on all CPUs.
    //
    // Note:
    //   IPI callbacks run at IRQL IPI_LEVEL,
    //   At that level, we cannot call ExAllocatePoolWithTag,
    //   therefore we have to use hypervisor allocator.
    //
    auto start_err = std::atomic<error_code_t>{};

    mp::ipi_call([&start_err]() {
      mm::allocator_guard _;

      const auto idx = mp::cpu_index();
      const auto err = global.vcpu_list[idx].start();

      auto expected = error_code_t{};
      start_err.compare_exchange_strong(expected, err);
    });

    //
    // Signalize that hypervisor has started.
    //
    global.running = true;

    //
    // If any VCPU started with an error, stop
    // the hypervisor.
    //
    // Note:
    //   This check must come AFTER setting "running"
    //   to true, otherwise "stop()" would perform
    //   early-exit without any actual stopping.
    //
    if (auto err = start_err.load())
    {
      stop();
      return err;
    }

    return {};
  }

  void stop() noexcept
  {
    //
    // If hypervisor is already stopped,
    // don't do anything.
    //
    hvpp_assert(global.running);
    if (!global.running)
    {
      return;
    }

    //
    // Stop virtualization on all CPUs.
    //
    mp::ipi_call([]() {
      mm::allocator_guard _;

      const auto idx = mp::cpu_index();
      global.vcpu_list[idx].stop();
    });

    //
    // Destroy array of VCPUs.
    //
    std::destroy_n(global.vcpu_list, mp::cpu_count());
    delete static_cast<void*>(global.vcpu_list);

    global.vcpu_list = nullptr;

    //
    // Signalize that hypervisor has stopped.
    //
    global.running = false;
  }

  bool is_running() noexcept
  {
    return global.running;
  }
}
