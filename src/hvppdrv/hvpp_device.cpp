#include "hvpp_device.h"

#include "win32/trace.h"

#include <hvpp/lib/assert.h>
#include <hvpp/lib/debugger.h>
#include <hvpp/lib/log.h>

#include <ntddk.h>

auto hvpp_device::initialize() noexcept -> error_code_t
{
  //
  // Initialize base class.
  //
  if (auto err = device::initialize())
  {
    destroy();
    return err;
  }

  //
  // Create VM-exit handler instance.
  //
  vmexit_handler_ = new vmexit_handler_t();

  if (!vmexit_handler_)
  {
    destroy();
    return make_error_code_t(std::errc::not_enough_memory);
  }

  //
  // Initialize VM-exit handler.
  //
  if (auto err = vmexit_handler_->initialize())
  {
    destroy();
    return err;
  }

  //
  // Create hypervisor instance.
  //
  hypervisor_ = new hypervisor();

  if (!hypervisor_)
  {
    destroy();
    return make_error_code_t(std::errc::not_enough_memory);
  }

  //
  // Initialize hypervisor instance.
  //
  if (auto err = hypervisor_->initialize())
  {
    destroy();
    return err;
  }

  hypervisor_->start(vmexit_handler_);

  //
  // Initialize monitor.
  //
  if (auto err = monitor::initialize())
  {
    destroy();
    return err;
  }

  return error_code_t{};
}

void hvpp_device::destroy() noexcept
{
  monitor::destroy();

  if (hypervisor_)
  {
    hypervisor_->destroy();
    delete hypervisor_;
  }

  if (vmexit_handler_)
  {
    vmexit_handler_->destroy();
    delete vmexit_handler_;
  }

  //
  // Destroy base class.
  //

  device::destroy();
}

error_code_t hvpp_device::on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept
{
  switch (code)
  {
    case ioctl_monitor_start_t::code():
      return ioctl_monitor_start(buffer, buffer_size);

    case ioctl_monitor_stop_t::code():
      return ioctl_monitor_stop(buffer, buffer_size);

    case ioctl_hypervisor_start_t::code():
      return ioctl_hypervisor_enable(buffer, buffer_size);

    case ioctl_hypervisor_stop_t::code():
      return ioctl_hypervisor_disable(buffer, buffer_size);

    case ioctl_shadow_page_add_t::code():
      return ioctl_shadow_page_add(buffer, buffer_size);

    case ioctl_shadow_page_apply_t::code():
      return ioctl_shadow_page_apply(buffer, buffer_size);

    default:
      //hvpp_assert(0);
      return make_error_code_t(std::errc::invalid_argument);
  }
}

error_code_t hvpp_device::ioctl_monitor_start(void* buffer, size_t buffer_size)
{
  hvpp_assert(buffer);
  hvpp_assert(buffer_size >= ioctl_monitor_start_t::size());

  if (!buffer || buffer_size < ioctl_monitor_start_t::size())
  {
    return make_error_code_t(std::errc::invalid_argument);
  }

  uint64_t& pid = *((uint64_t*)buffer);

  return monitor::start(pid);
}

error_code_t hvpp_device::ioctl_monitor_stop(void* buffer, size_t buffer_size)
{
  (void)(buffer);
  (void)(buffer_size);

  monitor::stop();

  return error_code_t{};
}

error_code_t hvpp_device::ioctl_hypervisor_enable(void* buffer, size_t buffer_size)
{
  (void)(buffer);
  (void)(buffer_size);

  return hypervisor_->start(vmexit_handler_);
}

error_code_t hvpp_device::ioctl_hypervisor_disable(void* buffer, size_t buffer_size)
{
  (void)(buffer);
  (void)(buffer_size);

  hypervisor_->stop();

  return error_code_t{};
}

error_code_t hvpp_device::ioctl_shadow_page_add(void* buffer, size_t buffer_size)
{
  hvpp_assert(buffer);
  hvpp_assert(buffer_size >= ioctl_shadow_page_add_t::size());

  if (!buffer || buffer_size < ioctl_shadow_page_add_t::size())
  {
    return make_error_code_t(std::errc::invalid_argument);
  }

  auto& params = *((ioctl_shadow_page_add_params*)buffer);

  //
  // TODO:
  // Log & lock - figure out if unlock manually or automatically?
  //

  PMDL ReadWriteMdl = IoAllocateMdl((void*)params.read_write, PAGE_SIZE, FALSE, FALSE, NULL);
  PMDL ExecuteMdl = IoAllocateMdl((void*)params.execute, PAGE_SIZE, FALSE, FALSE, NULL);
  MmProbeAndLockPages(ReadWriteMdl, UserMode, IoReadAccess);
  MmProbeAndLockPages(ExecuteMdl, UserMode, IoReadAccess);

  hvpp_info("read_write: %p (%p)", (void*)params.read_write, pa_t::from_va((void*)params.read_write).value());
  hvpp_info("execute:    %p (%p)", (void*)params.execute, pa_t::from_va((void*)params.execute).value());
  hvpp_info("read_write: 0x%x", params.offset);

  std::get<hvpp_vmexit_handler>(vmexit_handler_->handlers)
    .shadow_page_list()
    .push_back(shadow_page{
      pa_t::from_va((void*)params.read_write),
      pa_t::from_va((void*)params.execute),
      params.offset
    });

  return error_code_t{};
}

error_code_t hvpp_device::ioctl_shadow_page_apply(void* buffer, size_t buffer_size)
{
  (void)(buffer);
  (void)(buffer_size);

  vmx::vmcall(vmcall_apply_shadow_page);

  return error_code_t{};
}
