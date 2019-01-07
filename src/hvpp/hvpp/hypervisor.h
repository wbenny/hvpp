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

    auto start(vmexit_handler& handler) noexcept -> error_code_t;
    void stop() noexcept;

    bool is_started() const noexcept;
    auto exit_handler() noexcept -> vmexit_handler&;

  private:
    bool check_cpu_features() noexcept;

    void start_ipi_callback() noexcept;
    void stop_ipi_callback() noexcept;
    void check_ipi_callback() noexcept;

    vcpu_t* vcpu_list_;
    vmexit_handler* exit_handler_;
    bool check_passed_;
    bool started_;
};

}
