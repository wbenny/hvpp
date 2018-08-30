#pragma once
#include "hvpp/config.h"
#include "hvpp/vcpu.h"
#include "hvpp/vmexit.h"
#include "hvpp/vmexit_stats.h"

using namespace ia32;
using namespace hvpp;

#ifdef HVPP_ENABLE_STATS
using vmexit_base_handler = vmexit_stats_handler;
#else
using vmexit_base_handler = vmexit_handler;
#endif

class custom_vmexit_handler
  : public vmexit_base_handler
{
  public:
    void setup(vcpu_t& vp) noexcept override;

    void handle_execute_cpuid(vcpu_t& vp) noexcept override;
    void handle_execute_vmcall(vcpu_t& vp) noexcept override;
    void handle_ept_violation(vcpu_t& vp) noexcept override;

  private:
    struct per_vcpu_data
    {
      pa_t page_read;
      pa_t page_exec;
    };

    per_vcpu_data data_[32];
};
