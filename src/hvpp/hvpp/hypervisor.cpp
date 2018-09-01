#include "hypervisor.h"
#include "config.h"

#include "ia32/cpuid/cpuid_eax_01.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "lib/mp.h"

#ifdef HVPP_SINGLE_VCPU
# include <ntddk.h>

# define single_cpu_call(callback)                       \
   do                                                    \
   {                                                     \
     auto idx = 0;                                       \
     KeSetSystemAffinityThread((ULONG_PTR)1 << (idx));   \
     callback();                                         \
     KeRevertToUserAffinityThread();                     \
   } while (0)
#endif

namespace hvpp {

void hypervisor::initialize() noexcept
{
  vcpu_list_ = new vcpu_t[mp::cpu_count()];
  handler_ = nullptr;
  check_ = false;
}

void hypervisor::destroy() noexcept
{
  delete[] vcpu_list_;
}

bool hypervisor::check() noexcept
{
  if (!vcpu_list_)
  {
    return false;
  }

#ifdef HVPP_SINGLE_VCPU
  single_cpu_call(check_ipi_callback);
#else
  mp::ipi_call(this, &hypervisor::check_ipi_callback);
#endif

  return check_;
}

void hypervisor::start(vmexit_handler* handler) noexcept
{
  hvpp_assert(vcpu_list_ && check_ && handler);

  handler_ = handler;

#ifdef HVPP_SINGLE_VCPU
  single_cpu_call(start_ipi_callback);
#else
  mp::ipi_call(this, &hypervisor::start_ipi_callback);
#endif
}

void hypervisor::stop() noexcept
{
#ifdef HVPP_SINGLE_VCPU
  single_cpu_call(stop_ipi_callback);
#else
  mp::ipi_call(this, &hypervisor::stop_ipi_callback);
#endif
}

//
// Private
//

void hypervisor::check_ipi_callback() noexcept
{
  cpuid_eax_01 cpuid_info;
  ia32_asm_cpuid(cpuid_info.cpu_info, 1);
  if (!cpuid_info.feature_information_ecx.virtual_machine_extensions)
  {
    return;
  }

  auto cr4 = read<cr4_t>();
  if (cr4.vmx_enable)
  {
    return;
  }

  auto vmx_basic = msr::read<msr::vmx_basic_t>();
  if (
     vmx_basic.vmcs_size_in_bytes > page_size ||
     vmx_basic.memory_type != static_cast<uint64_t>(memory_type::write_back) ||
    !vmx_basic.true_controls
    )
  {
    return;
  }

  auto vmx_ept_vpid_cap = msr::read<msr::vmx_ept_vpid_cap_t>();
  if (
    !vmx_ept_vpid_cap.page_walk_length_4      ||
    !vmx_ept_vpid_cap.memory_type_write_back  ||
    !vmx_ept_vpid_cap.invept                  ||
    !vmx_ept_vpid_cap.invept_all_contexts     ||
    !vmx_ept_vpid_cap.execute_only_pages      ||
    !vmx_ept_vpid_cap.pde_2mb_pages
    )
  {
    return;
  }

  check_ = true;
}

void hypervisor::start_ipi_callback() noexcept
{
  auto idx = mp::cpu_index();
  vcpu_list_[idx].initialize(handler_);
  vcpu_list_[idx].launch();
}

void hypervisor::stop_ipi_callback() noexcept
{
  auto idx = mp::cpu_index();
  vcpu_list_[idx].destroy();
}

}
