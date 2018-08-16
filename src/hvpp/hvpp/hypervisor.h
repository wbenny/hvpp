#pragma once
#include "vcpu.h"
#include "vmexit.h"

namespace hvpp {

using namespace ia32;

class hypervisor
{
  public:
    void initialize() noexcept;
    void destroy() noexcept;

    bool check() noexcept;

    void start(vmexit_handler* handler) noexcept;
    void stop() noexcept;

  private:
    void check_ipi_callback() noexcept;

    void start_ipi_callback() noexcept;
    void stop_ipi_callback() noexcept;

    vcpu_t vcpu_[32];
    vmexit_handler* handler_;
    bool check_;
};

}
