#pragma once

namespace hvpp {

class vcpu;

class vmexit_handler
{
  public:
    vmexit_handler();

    virtual void handle(vcpu* vp) noexcept;
    virtual void invoke_termination() noexcept;

  protected:
    virtual void handle_exception_or_nmi(vcpu* vp) noexcept;
    virtual void handle_external_interrupt(vcpu* vp) noexcept;
    virtual void handle_triple_fault(vcpu* vp) noexcept;
    virtual void handle_init_signal(vcpu* vp) noexcept;
    virtual void handle_startup_ipi(vcpu* vp) noexcept;
    virtual void handle_io_smi(vcpu* vp) noexcept;
    virtual void handle_smi(vcpu* vp) noexcept;
    virtual void handle_interrupt_window(vcpu* vp) noexcept;
    virtual void handle_nmi_window(vcpu* vp) noexcept;
    virtual void handle_task_switch(vcpu* vp) noexcept;
    virtual void handle_execute_cpuid(vcpu* vp) noexcept;
    virtual void handle_execute_getsec(vcpu* vp) noexcept;
    virtual void handle_execute_hlt(vcpu* vp) noexcept;
    virtual void handle_execute_invd(vcpu* vp) noexcept;
    virtual void handle_execute_invlpg(vcpu* vp) noexcept;
    virtual void handle_execute_rdpmc(vcpu* vp) noexcept;
    virtual void handle_execute_rdtsc(vcpu* vp) noexcept;
    virtual void handle_execute_rsm_in_smm(vcpu* vp) noexcept;
    virtual void handle_execute_vmcall(vcpu* vp) noexcept;
    virtual void handle_execute_vmclear(vcpu* vp) noexcept;
    virtual void handle_execute_vmlaunch(vcpu* vp) noexcept;
    virtual void handle_execute_vmptrld(vcpu* vp) noexcept;
    virtual void handle_execute_vmptrst(vcpu* vp) noexcept;
    virtual void handle_execute_vmread(vcpu* vp) noexcept;
    virtual void handle_execute_vmresume(vcpu* vp) noexcept;
    virtual void handle_execute_vmwrite(vcpu* vp) noexcept;
    virtual void handle_execute_vmxoff(vcpu* vp) noexcept;
    virtual void handle_execute_vmxon(vcpu* vp) noexcept;
    virtual void handle_mov_cr(vcpu* vp) noexcept;
    virtual void handle_mov_dr(vcpu* vp) noexcept;
    virtual void handle_execute_io_instruction(vcpu* vp) noexcept;
    virtual void handle_execute_rdmsr(vcpu* vp) noexcept;
    virtual void handle_execute_wrmsr(vcpu* vp) noexcept;
    virtual void handle_error_invalid_guest_state(vcpu* vp) noexcept;
    virtual void handle_error_msr_load(vcpu* vp) noexcept;
    virtual void handle_execute_mwait(vcpu* vp) noexcept;
    virtual void handle_monitor_trap_flag(vcpu* vp) noexcept;
    virtual void handle_execute_monitor(vcpu* vp) noexcept;
    virtual void handle_execute_pause(vcpu* vp) noexcept;
    virtual void handle_error_machine_check(vcpu* vp) noexcept;
    virtual void handle_tpr_below_threshold(vcpu* vp) noexcept;
    virtual void handle_apic_access(vcpu* vp) noexcept;
    virtual void handle_virtualized_eoi(vcpu* vp) noexcept;
    virtual void handle_gdtr_idtr_access(vcpu* vp) noexcept;
    virtual void handle_ldtr_tr_access(vcpu* vp) noexcept;
    virtual void handle_ept_violation(vcpu* vp) noexcept;
    virtual void handle_ept_misconfiguration(vcpu* vp) noexcept;
    virtual void handle_execute_invept(vcpu* vp) noexcept;
    virtual void handle_execute_rdtscp(vcpu* vp) noexcept;
    virtual void handle_vmx_preemption_timer_expired(vcpu* vp) noexcept;
    virtual void handle_execute_invvpid(vcpu* vp) noexcept;
    virtual void handle_execute_wbinvd(vcpu* vp) noexcept;
    virtual void handle_execute_xsetbv(vcpu* vp) noexcept;
    virtual void handle_apic_write(vcpu* vp) noexcept;
    virtual void handle_execute_rdrand(vcpu* vp) noexcept;
    virtual void handle_execute_invpcid(vcpu* vp) noexcept;
    virtual void handle_execute_vmfunc(vcpu* vp) noexcept;
    virtual void handle_execute_encls(vcpu* vp) noexcept;
    virtual void handle_execute_rdseed(vcpu* vp) noexcept;
    virtual void handle_page_modification_log_full(vcpu* vp) noexcept;
    virtual void handle_execute_xsaves(vcpu* vp) noexcept;
    virtual void handle_execute_xrstors(vcpu* vp) noexcept;

    virtual void handle_fallback(vcpu* vp) noexcept { UNREFERENCED_PARAMETER(vp); }
    virtual void handle_execute_vm_fallback(vcpu* vp) noexcept;

  private:
    using handler_fn_t = void (vmexit_handler::*)(vcpu*);
    handler_fn_t handlers_[64];
};

}
