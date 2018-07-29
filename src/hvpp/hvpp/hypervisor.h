#pragma once
#include "vcpu.h"

namespace hvpp {

using namespace ia32;

class hypervisor
{
  public:
    void check() noexcept;

    void start() noexcept;
    void stop() noexcept;

  private:
    void check_ipi_callback() noexcept;

    void start_ipi_callback() noexcept;
    void stop_ipi_callback() noexcept;

    vcpu vcpu_[32];
    bool check_;
};

}
