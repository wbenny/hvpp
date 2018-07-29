#pragma once
#include "ept.h"
#include "vmexit.h"
#include "ia32/arch.h"
#include "ia32/exception.h"
#include "ia32/interrupt.h"
#include "ia32/vmx.h"
#include <cstdint>

namespace hvpp {

using namespace ia32;

static constexpr int vcpu_stack_size = 0x8000;

enum class vcpu_state
{
  //
  // VCPU is unitialized.
  //
  off,

  //
  // VCPU is in VMX root mode; host & guest VMCS is being initialized.
  //
  initializing,

  //
  // VCPU successfully performed its initial VMENTRY.
  //
  launching,

  //
  // VCPU is running.
  //
  running,

  //
  // VCPU is terminating; vcpu::destroy has been called.
  //
  terminating,

  //
  // VCPU is terminated, VMX root mode has been left.
  //
  terminated,
};

class interrupt_info
{
  public:
    interrupt_info(vmx::interrupt_type intr_type, exception_vector expt_vector,
                   int rip_adjust = -1) noexcept
      : interrupt_info(intr_type, expt_vector, exception_error_code{ 0 }, false, rip_adjust) { }

    interrupt_info(vmx::interrupt_type intr_type, exception_vector expt_vector,
                   exception_error_code expt_code, int rip_adjust = -1) noexcept
      : interrupt_info(intr_type, expt_vector, expt_code, true, rip_adjust) { }

    interrupt_info(const interrupt_info& other) noexcept = default;
    interrupt_info(interrupt_info&& other) noexcept = default;
    interrupt_info& operator=(const interrupt_info& other) noexcept = default;
    interrupt_info& operator=(interrupt_info&& other) noexcept = default;

    auto vector() const noexcept { return static_cast<exception_vector>(info_.vector); }
    auto type() const noexcept { return static_cast<vmx::interrupt_type>(info_.type); }
    auto error_code() const noexcept { return error_code_; }
    int  rip_adjust() const noexcept { return rip_adjust_; }

    bool error_code_valid() const noexcept { return info_.error_code_valid; }
    bool nmi_unblocking() const noexcept { return info_.nmi_unblocking; }
    bool valid() const noexcept { return info_.valid; }

  private:
    friend class vcpu;

    interrupt_info() noexcept : info_(), error_code_(), rip_adjust_() { }
    interrupt_info(
      vmx::interrupt_type intr_type,
      exception_vector expt_vector,
      exception_error_code expt_code,
      bool expt_code_valid,
      int rip_adjust
      ) noexcept
    {
      info_.flags = 0;

      info_.vector = static_cast<uint32_t>(expt_vector);
      info_.type   = static_cast<uint32_t>(intr_type);
      info_.valid  = true;

      //
      // Final sanitization of the following fields takes place in vcpu::inject().
      //

      info_.error_code_valid = expt_code_valid;
      error_code_  = expt_code;
      rip_adjust_  = rip_adjust;
    }

    vmx::interrupt_info info_;
    exception_error_code error_code_;
    int rip_adjust_;
};

class vcpu
{
  public:
    void initialize() noexcept;
    void destroy() noexcept;

    void launch() noexcept;
    void terminate() noexcept;

    void set_exit_handler(vmexit_handler* exit_handler) noexcept;

    //
    // Control state
    //

    auto pin_based_controls() const noexcept -> msr::vmx_pinbased_ctls;
    void pin_based_controls(msr::vmx_pinbased_ctls controls) noexcept;

    auto processor_based_controls() const noexcept -> msr::vmx_procbased_ctls;
    void processor_based_controls(msr::vmx_procbased_ctls controls) noexcept;

    auto processor_based_controls2() const noexcept -> msr::vmx_procbased_ctls2;
    void processor_based_controls2(msr::vmx_procbased_ctls2 controls) noexcept;

    auto vm_entry_controls() const noexcept -> msr::vmx_entry_ctls;
    void vm_entry_controls(msr::vmx_entry_ctls controls) noexcept;

    auto vm_exit_controls() const noexcept -> msr::vmx_exit_ctls;
    void vm_exit_controls(msr::vmx_exit_ctls controls) noexcept;

    auto exception_bitmap() const noexcept -> vmx::exception_bitmap;
    void exception_bitmap(vmx::exception_bitmap exception_bitmap) noexcept;
    auto msr_bitmap() const noexcept -> const vmx::msr_bitmap&;
    void msr_bitmap(const vmx::msr_bitmap& msr_bitmap) noexcept;
    auto io_bitmap() const noexcept -> const vmx::io_bitmap&;
    void io_bitmap(const vmx::io_bitmap& io_bitmap) noexcept;

    auto pagefault_error_code_mask() const noexcept -> pagefault_error_code;
    void pagefault_error_code_mask(pagefault_error_code mask) noexcept;
    auto pagefault_error_code_match() const noexcept -> pagefault_error_code;
    void pagefault_error_code_match(pagefault_error_code match) noexcept;


    //
    // Control entry state
    //

    void inject(interrupt_info interrupt) noexcept;
    void suppress_rip_adjust() noexcept;

    auto entry_instruction_length() const noexcept -> uint32_t;
    void entry_instruction_length(uint32_t instruction_length) noexcept;

    auto entry_interruption_info() const noexcept -> vmx::interrupt_info;
    void entry_interruption_info(vmx::interrupt_info info) noexcept;

    auto entry_interruption_error_code() const noexcept -> exception_error_code;
    void entry_interruption_error_code(exception_error_code error_code) noexcept;

    //
    // Exit state
    //

    auto exit_interrupt_info() const noexcept -> interrupt_info;

    auto exit_instruction_error() const noexcept -> vmx::instruction_error;
    auto exit_instruction_info() const noexcept -> uint32_t;
    auto exit_instruction_length() const noexcept -> uint32_t;

    auto exit_interruption_info() const noexcept -> vmx::interrupt_info;
    auto exit_interruption_error_code() const noexcept -> exception_error_code;

    auto exit_reason() const noexcept -> vmx::exit_reason;
    auto exit_qualification() const noexcept -> vmx::exit_qualification;
    auto exit_guest_physical_address() const noexcept -> pa_t;
    auto exit_guest_linear_address() const noexcept -> la_t;

    auto exit_context() noexcept -> context&;

    //
    // Guest state
    //

    auto guest_cr0_shadow() const noexcept -> cr0_t;
    void guest_cr0_shadow(cr0_t cr0) noexcept;
    auto guest_cr0() const noexcept -> cr0_t;
    void guest_cr0(cr0_t cr0) noexcept;
    auto guest_cr3() const noexcept -> cr3_t;
    void guest_cr3(cr3_t cr3) noexcept;
    auto guest_cr4_shadow() const noexcept -> cr4_t;
    void guest_cr4_shadow(cr4_t cr4) noexcept;
    auto guest_cr4() const noexcept -> cr4_t;
    void guest_cr4(cr4_t cr4) noexcept;

    auto guest_dr7() const noexcept -> dr7_t;
    void guest_dr7(dr7_t dr7) noexcept;
    auto guest_debugctl() const noexcept -> msr::debugctl;
    void guest_debugctl(msr::debugctl debugctl) noexcept;

    auto guest_rsp() const noexcept -> uint64_t;
    void guest_rsp(uint64_t rsp) noexcept;
    auto guest_rip() const noexcept -> uint64_t;
    void guest_rip(uint64_t rip) noexcept;
    auto guest_rflags() const noexcept -> rflags_t;
    void guest_rflags(rflags_t rflags) noexcept;

    auto guest_gdtr() const noexcept -> gdtr_t;
    void guest_gdtr(gdtr_t gdtr) noexcept;
    auto guest_idtr() const noexcept -> idtr_t;
    void guest_idtr(idtr_t idtr) noexcept;

    auto guest_cs() const noexcept -> seg_t<cs_t>;
    void guest_cs(seg_t<cs_t> cs) noexcept;
    auto guest_ds() const noexcept -> seg_t<ds_t>;
    void guest_ds(seg_t<ds_t> ds) noexcept;
    auto guest_es() const noexcept -> seg_t<es_t>;
    void guest_es(seg_t<es_t> es) noexcept;
    auto guest_fs() const noexcept -> seg_t<fs_t>;
    void guest_fs(seg_t<fs_t> fs) noexcept;
    auto guest_gs() const noexcept -> seg_t<gs_t>;
    void guest_gs(seg_t<gs_t> gs) noexcept;
    auto guest_ss() const noexcept -> seg_t<ss_t>;
    void guest_ss(seg_t<ss_t> ss) noexcept;
    auto guest_tr() const noexcept -> seg_t<tr_t>;
    void guest_tr(seg_t<tr_t> tr) noexcept;
    auto guest_ldtr() const noexcept -> seg_t<ldtr_t>;
    void guest_ldtr(seg_t<ldtr_t> ldtr) noexcept;

  private:

    //
    // Host state
    //

    auto host_cr0() const noexcept -> cr0_t;
    void host_cr0(cr0_t cr0) noexcept;
    auto host_cr3() const noexcept -> cr3_t;
    void host_cr3(cr3_t cr3) noexcept;
    auto host_cr4() const noexcept -> cr4_t;
    void host_cr4(cr4_t cr4) noexcept;

    auto host_rsp() const noexcept -> uint64_t;
    void host_rsp(uint64_t rsp) noexcept;
    auto host_rip() const noexcept -> uint64_t;
    void host_rip(uint64_t rip) noexcept;

    auto host_gdtr() const noexcept -> gdtr_t;
    void host_gdtr(gdtr_t gdtr) noexcept;
    auto host_idtr() const noexcept -> idtr_t;
    void host_idtr(idtr_t idtr) noexcept;

    auto host_cs() const noexcept -> seg_t<cs_t>;
    void host_cs(seg_t<cs_t> cs) noexcept;
    auto host_ds() const noexcept -> seg_t<ds_t>;
    void host_ds(seg_t<ds_t> ds) noexcept;
    auto host_es() const noexcept -> seg_t<es_t>;
    void host_es(seg_t<es_t> es) noexcept;
    auto host_fs() const noexcept -> seg_t<fs_t>;
    void host_fs(seg_t<fs_t> fs) noexcept;
    auto host_gs() const noexcept -> seg_t<gs_t>;
    void host_gs(seg_t<gs_t> gs) noexcept;
    auto host_ss() const noexcept -> seg_t<ss_t>;
    void host_ss(seg_t<ss_t> ss) noexcept;
    auto host_tr() const noexcept -> seg_t<tr_t>;
    void host_tr(seg_t<tr_t> tr) noexcept;

    //
    // ldtr does not exist in VMX root mode.
    //
    // auto host_ldtr() const noexcept -> seg_t<ldtr_t>;
    // void host_ldtr(seg_t<ldtr_t> ldtr)  noexcept;
    //

  private:
    void error() noexcept;
    void setup() noexcept;

    void load_vmcs_host() noexcept;
    void load_vmcs_guest() noexcept;

    void setup_vmcs_host() noexcept;
    void setup_vmcs_guest() noexcept;

    void entry_host() noexcept;
    void entry_guest() noexcept;

    static void entry_host_() noexcept;
    static void entry_guest_() noexcept;

    alignas(PAGE_SIZE) uint8_t            stack_[vcpu_stack_size];
                       context            guest_context_;
                       context            exit_context_;
                       vcpu_state         state_;
                       vmx::instruction_error state_instruction_error_;
                       ept                ept_;
                       vmexit_handler*    exit_handler_;
                       vmexit_handler     exit_handler_default_;
                       bool               suppress_rip_adjust_;
    alignas(PAGE_SIZE) vmx::vmcs          vmcs_host_;
    alignas(PAGE_SIZE) vmx::vmcs          vmcs_guest_;
    alignas(PAGE_SIZE) vmx::msr_bitmap    msr_bitmap_;
    alignas(PAGE_SIZE) vmx::io_bitmap     io_bitmap_;
};

}
