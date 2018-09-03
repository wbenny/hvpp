#pragma once
#include "vcpu.h"
#include "vmexit.h"

#include "lib/error.h"

namespace hvpp {

using namespace ia32;

class hypervisor
{
  public:
    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;

    void start(vmexit_handler* handler) noexcept;
    void stop() noexcept;

  private:
    bool check_cpu_features() noexcept;

    void start_ipi_callback() noexcept;
    void stop_ipi_callback() noexcept;
    void check_ipi_callback() noexcept;

    vcpu_t* vcpu_list_;
    vmexit_handler* handler_;
    bool check_passed_;
};

}
