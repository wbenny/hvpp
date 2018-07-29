#include "hypervisor.h"
#include "ept.h"
#include "vcpu.h"
#include "lib/bitmap.h"
#include "lib/mp.h"

#include <iterator>

namespace hvpp {

#define HV_UP 0

void hypervisor::check() noexcept
{
  check_ = false;

#if HV_UP
  auto idx = 0;
  KeSetSystemAffinityThread((ULONG_PTR)1 << (idx));
  check_ipi_callback();
  KeRevertToUserAffinityThread();
#else
  mp::ipi_call(this, &hypervisor::check_ipi_callback);
#endif
}

void hypervisor::start() noexcept
{
#if HV_UP
  auto idx = 0;
  KeSetSystemAffinityThread((ULONG_PTR)1 << (idx));
  start_ipi_callback();
  KeRevertToUserAffinityThread();
#else
  mp::ipi_call(this, &hypervisor::start_ipi_callback);
#endif
}

void hypervisor::stop() noexcept
{
#if HV_UP
  auto idx = 0;
  KeSetSystemAffinityThread((ULONG_PTR)1 << (idx));
  stop_ipi_callback();
  KeRevertToUserAffinityThread();
#else
  mp::ipi_call(this, &hypervisor::stop_ipi_callback);
#endif
}

//
// Private
//

void hypervisor::start_ipi_callback() noexcept
{
  auto idx = mp::cpu_index();
  vcpu_[idx].initialize();
  vcpu_[idx].launch();
}

void hypervisor::stop_ipi_callback() noexcept
{
  auto idx = mp::cpu_index();
  vcpu_[idx].destroy();
  //vcpu_[idx].terminate();
}

void hypervisor::check_ipi_callback() noexcept
{
  auto cr4 = read<cr4_t>();
  if (cr4.vmx_enable)
  {
    return;
  }

  auto vmx_basic = msr::read<msr::vmx_basic>();
  if (
     vmx_basic.vmcs_size_in_bytes > PAGE_SIZE ||
     vmx_basic.memory_type        != static_cast<uint64_t>(mtype::write_back) ||
    !vmx_basic.true_controls
    )
  {
    return;
  }

  auto vmx_ept_vpid_cap = msr::read<msr::vmx_ept_vpid_cap>();
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

}
