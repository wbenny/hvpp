#pragma once
#include "hvpp_vmexit_handler.h"
#include "monitor.h"

#include <hvpp/hypervisor.h>
#include <hvpp/vmexit_compositor.h>
#include <hvpp/lib/device.h>

#include <cstdint>

using namespace ia32;
using namespace hvpp;

//
// Create combined handler from these VM-exit handlers.
//
using vmexit_handler_t = vmexit_compositor_handler<
  vmexit_stats_handler,
  hvpp_vmexit_handler
>;

static_assert(std::is_base_of_v<vmexit_handler, vmexit_handler_t>);

using ioctl_monitor_start_t     = ioctl_read_t<1, sizeof(uint64_t)>;
using ioctl_monitor_stop_t      = ioctl_none_t<2>;

using ioctl_hypervisor_start_t  = ioctl_none_t<3>;
using ioctl_hypervisor_stop_t   = ioctl_none_t<4>;

struct ioctl_shadow_page_add_params
{
  uint64_t read_write;
  uint64_t execute;
  uint64_t offset;
};

using ioctl_shadow_page_add_t   = ioctl_read_t<5, sizeof(ioctl_shadow_page_add_params)>;
using ioctl_shadow_page_apply_t = ioctl_none_t<6>;

class hvpp_device
  : public device
{
  public:
    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;

    const char* name() const noexcept override { return "hvpp"; }

    error_code_t on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept override;

  private:
    error_code_t ioctl_monitor_start(void* buffer, size_t buffer_size);
    error_code_t ioctl_monitor_stop(void* buffer, size_t buffer_size);
    error_code_t ioctl_hypervisor_enable(void* buffer, size_t buffer_size);
    error_code_t ioctl_hypervisor_disable(void* buffer, size_t buffer_size);
    error_code_t ioctl_shadow_page_add(void* buffer, size_t buffer_size);
    error_code_t ioctl_shadow_page_apply(void* buffer, size_t buffer_size);

    hypervisor* hypervisor_;
    vmexit_handler_t* vmexit_handler_;
};
