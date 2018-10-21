#pragma once
#include <hvpp/config.h>
#include <hvpp/vcpu.h>
#include <hvpp/vmexit.h>
#include <hvpp/vmexit/vmexit_stats.h>
#include <hvpp/vmexit/vmexit_dbgbreak.h>
#include <hvpp/vmexit/vmexit_passthrough.h>

#include <hvpp/lib/list.h>

using namespace ia32;
using namespace hvpp;

struct shadow_page
{
  pa_t read_write;
  pa_t execute;

  uint64_t offset;
};

static constexpr uint64_t vmcall_apply_shadow_page = 0x01;

class hvpp_vmexit_handler
  : public vmexit_passthrough_handler
{
  public:
    using base_type = vmexit_passthrough_handler;

    void setup(vcpu_t& vp) noexcept override;

    void handle_execute_cpuid(vcpu_t& vp) noexcept override;
    void handle_execute_vmcall(vcpu_t& vp) noexcept override;
    void handle_ept_violation(vcpu_t& vp) noexcept override;

    list<shadow_page>& shadow_page_list() noexcept { return shadow_page_; }

  private:
    list<shadow_page> shadow_page_;

    struct per_vcpu_data
    {
      pa_t page_read;
      pa_t page_exec;
    };

    per_vcpu_data data_[32];
};
