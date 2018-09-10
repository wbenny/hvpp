#include "vmexit.h"
#include "vcpu.h"

#include "ia32/vmx.h"

namespace hvpp {

vmexit_handler::vmexit_handler() noexcept
  : handlers_{ {
    &vmexit_handler::handle_exception_or_nmi,
    &vmexit_handler::handle_external_interrupt,
    &vmexit_handler::handle_triple_fault,
    &vmexit_handler::handle_init_signal,
    &vmexit_handler::handle_startup_ipi,
    &vmexit_handler::handle_io_smi,
    &vmexit_handler::handle_smi,
    &vmexit_handler::handle_interrupt_window,
    &vmexit_handler::handle_nmi_window,
    &vmexit_handler::handle_task_switch,
    &vmexit_handler::handle_execute_cpuid,
    &vmexit_handler::handle_execute_getsec,
    &vmexit_handler::handle_execute_hlt,
    &vmexit_handler::handle_execute_invd,
    &vmexit_handler::handle_execute_invlpg,
    &vmexit_handler::handle_execute_rdpmc,
    &vmexit_handler::handle_execute_rdtsc,
    &vmexit_handler::handle_execute_rsm_in_smm,
    &vmexit_handler::handle_execute_vmcall,
    &vmexit_handler::handle_execute_vmclear,
    &vmexit_handler::handle_execute_vmlaunch,
    &vmexit_handler::handle_execute_vmptrld,
    &vmexit_handler::handle_execute_vmptrst,
    &vmexit_handler::handle_execute_vmread,
    &vmexit_handler::handle_execute_vmresume,
    &vmexit_handler::handle_execute_vmwrite,
    &vmexit_handler::handle_execute_vmxoff,
    &vmexit_handler::handle_execute_vmxon,
    &vmexit_handler::handle_mov_cr,
    &vmexit_handler::handle_mov_dr,
    &vmexit_handler::handle_execute_io_instruction,
    &vmexit_handler::handle_execute_rdmsr,
    &vmexit_handler::handle_execute_wrmsr,
    &vmexit_handler::handle_error_invalid_guest_state,
    &vmexit_handler::handle_error_msr_load,
    &vmexit_handler::handle_fallback, // reserved_1
    &vmexit_handler::handle_execute_mwait,
    &vmexit_handler::handle_monitor_trap_flag,
    &vmexit_handler::handle_fallback, // reserved_2
    &vmexit_handler::handle_execute_monitor,
    &vmexit_handler::handle_execute_pause,
    &vmexit_handler::handle_error_machine_check,
    &vmexit_handler::handle_fallback, // reserved_3
    &vmexit_handler::handle_tpr_below_threshold,
    &vmexit_handler::handle_apic_access,
    &vmexit_handler::handle_virtualized_eoi,
    &vmexit_handler::handle_gdtr_idtr_access,
    &vmexit_handler::handle_ldtr_tr_access,
    &vmexit_handler::handle_ept_violation,
    &vmexit_handler::handle_ept_misconfiguration,
    &vmexit_handler::handle_execute_invept,
    &vmexit_handler::handle_execute_rdtscp,
    &vmexit_handler::handle_vmx_preemption_timer_expired,
    &vmexit_handler::handle_execute_invvpid,
    &vmexit_handler::handle_execute_wbinvd,
    &vmexit_handler::handle_execute_xsetbv,
    &vmexit_handler::handle_apic_write,
    &vmexit_handler::handle_execute_rdrand,
    &vmexit_handler::handle_execute_invpcid,
    &vmexit_handler::handle_execute_vmfunc,
    &vmexit_handler::handle_execute_encls,
    &vmexit_handler::handle_execute_rdseed,
    &vmexit_handler::handle_page_modification_log_full,
    &vmexit_handler::handle_execute_xsaves,
    &vmexit_handler::handle_execute_xrstors,
  } }
{
  handlers_[static_cast<int>(vmx::exit_reason::exception_or_nmi)]             = &vmexit_handler::handle_exception_or_nmi;
  handlers_[static_cast<int>(vmx::exit_reason::external_interrupt)]           = &vmexit_handler::handle_external_interrupt;
  handlers_[static_cast<int>(vmx::exit_reason::triple_fault)]                 = &vmexit_handler::handle_triple_fault;
  handlers_[static_cast<int>(vmx::exit_reason::init_signal)]                  = &vmexit_handler::handle_init_signal;
  handlers_[static_cast<int>(vmx::exit_reason::startup_ipi)]                  = &vmexit_handler::handle_startup_ipi;
  handlers_[static_cast<int>(vmx::exit_reason::io_smi)]                       = &vmexit_handler::handle_io_smi;
  handlers_[static_cast<int>(vmx::exit_reason::smi)]                          = &vmexit_handler::handle_smi;
  handlers_[static_cast<int>(vmx::exit_reason::interrupt_window)]             = &vmexit_handler::handle_interrupt_window;
  handlers_[static_cast<int>(vmx::exit_reason::nmi_window)]                   = &vmexit_handler::handle_nmi_window;
  handlers_[static_cast<int>(vmx::exit_reason::task_switch)]                  = &vmexit_handler::handle_task_switch;
  handlers_[static_cast<int>(vmx::exit_reason::execute_cpuid)]                = &vmexit_handler::handle_execute_cpuid;
  handlers_[static_cast<int>(vmx::exit_reason::execute_getsec)]               = &vmexit_handler::handle_execute_getsec;
  handlers_[static_cast<int>(vmx::exit_reason::execute_hlt)]                  = &vmexit_handler::handle_execute_hlt;
  handlers_[static_cast<int>(vmx::exit_reason::execute_invd)]                 = &vmexit_handler::handle_execute_invd;
  handlers_[static_cast<int>(vmx::exit_reason::execute_invlpg)]               = &vmexit_handler::handle_execute_invlpg;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdpmc)]                = &vmexit_handler::handle_execute_rdpmc;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdtsc)]                = &vmexit_handler::handle_execute_rdtsc;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rsm_in_smm)]           = &vmexit_handler::handle_execute_rsm_in_smm;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmcall)]               = &vmexit_handler::handle_execute_vmcall;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmclear)]              = &vmexit_handler::handle_execute_vmclear;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmlaunch)]             = &vmexit_handler::handle_execute_vmlaunch;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmptrld)]              = &vmexit_handler::handle_execute_vmptrld;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmptrst)]              = &vmexit_handler::handle_execute_vmptrst;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmread)]               = &vmexit_handler::handle_execute_vmread;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmresume)]             = &vmexit_handler::handle_execute_vmresume;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmwrite)]              = &vmexit_handler::handle_execute_vmwrite;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmxoff)]               = &vmexit_handler::handle_execute_vmxoff;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmxon)]                = &vmexit_handler::handle_execute_vmxon;
  handlers_[static_cast<int>(vmx::exit_reason::mov_cr)]                       = &vmexit_handler::handle_mov_cr;
  handlers_[static_cast<int>(vmx::exit_reason::mov_dr)]                       = &vmexit_handler::handle_mov_dr;
  handlers_[static_cast<int>(vmx::exit_reason::execute_io_instruction)]       = &vmexit_handler::handle_execute_io_instruction;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdmsr)]                = &vmexit_handler::handle_execute_rdmsr;
  handlers_[static_cast<int>(vmx::exit_reason::execute_wrmsr)]                = &vmexit_handler::handle_execute_wrmsr;
  handlers_[static_cast<int>(vmx::exit_reason::error_invalid_guest_state)]    = &vmexit_handler::handle_error_invalid_guest_state;
  handlers_[static_cast<int>(vmx::exit_reason::error_msr_load)]               = &vmexit_handler::handle_error_msr_load;
  handlers_[static_cast<int>(vmx::exit_reason::execute_mwait)]                = &vmexit_handler::handle_execute_mwait;
  handlers_[static_cast<int>(vmx::exit_reason::monitor_trap_flag)]            = &vmexit_handler::handle_monitor_trap_flag;
  handlers_[static_cast<int>(vmx::exit_reason::execute_monitor)]              = &vmexit_handler::handle_execute_monitor;
  handlers_[static_cast<int>(vmx::exit_reason::execute_pause)]                = &vmexit_handler::handle_execute_pause;
  handlers_[static_cast<int>(vmx::exit_reason::error_machine_check)]          = &vmexit_handler::handle_error_machine_check;
  handlers_[static_cast<int>(vmx::exit_reason::tpr_below_threshold)]          = &vmexit_handler::handle_tpr_below_threshold;
  handlers_[static_cast<int>(vmx::exit_reason::apic_access)]                  = &vmexit_handler::handle_apic_access;
  handlers_[static_cast<int>(vmx::exit_reason::virtualized_eoi)]              = &vmexit_handler::handle_virtualized_eoi;
  handlers_[static_cast<int>(vmx::exit_reason::gdtr_idtr_access)]             = &vmexit_handler::handle_gdtr_idtr_access;
  handlers_[static_cast<int>(vmx::exit_reason::ldtr_tr_access)]               = &vmexit_handler::handle_ldtr_tr_access;
  handlers_[static_cast<int>(vmx::exit_reason::ept_violation)]                = &vmexit_handler::handle_ept_violation;
  handlers_[static_cast<int>(vmx::exit_reason::ept_misconfiguration)]         = &vmexit_handler::handle_ept_misconfiguration;
  handlers_[static_cast<int>(vmx::exit_reason::execute_invept)]               = &vmexit_handler::handle_execute_invept;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdtscp)]               = &vmexit_handler::handle_execute_rdtscp;
  handlers_[static_cast<int>(vmx::exit_reason::vmx_preemption_timer_expired)] = &vmexit_handler::handle_vmx_preemption_timer_expired;
  handlers_[static_cast<int>(vmx::exit_reason::execute_invvpid)]              = &vmexit_handler::handle_execute_invvpid;
  handlers_[static_cast<int>(vmx::exit_reason::execute_wbinvd)]               = &vmexit_handler::handle_execute_wbinvd;
  handlers_[static_cast<int>(vmx::exit_reason::execute_xsetbv)]               = &vmexit_handler::handle_execute_xsetbv;
  handlers_[static_cast<int>(vmx::exit_reason::apic_write)]                   = &vmexit_handler::handle_apic_write;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdrand)]               = &vmexit_handler::handle_execute_rdrand;
  handlers_[static_cast<int>(vmx::exit_reason::execute_invpcid)]              = &vmexit_handler::handle_execute_invpcid;
  handlers_[static_cast<int>(vmx::exit_reason::execute_vmfunc)]               = &vmexit_handler::handle_execute_vmfunc;
  handlers_[static_cast<int>(vmx::exit_reason::execute_encls)]                = &vmexit_handler::handle_execute_encls;
  handlers_[static_cast<int>(vmx::exit_reason::execute_rdseed)]               = &vmexit_handler::handle_execute_rdseed;
  handlers_[static_cast<int>(vmx::exit_reason::page_modification_log_full)]   = &vmexit_handler::handle_page_modification_log_full;
  handlers_[static_cast<int>(vmx::exit_reason::execute_xsaves)]               = &vmexit_handler::handle_execute_xsaves;
  handlers_[static_cast<int>(vmx::exit_reason::execute_xrstors)]              = &vmexit_handler::handle_execute_xrstors;
}

vmexit_handler::~vmexit_handler() noexcept
{

}

auto vmexit_handler::initialize() noexcept -> error_code_t
{
  return error_code_t{};
}

void vmexit_handler::destroy() noexcept
{

}

void vmexit_handler::setup(vcpu_t& vp) noexcept
{
  (void)vp;
}

void vmexit_handler::handle(vcpu_t& vp) noexcept
{
  auto handler_index = static_cast<int>(vp.exit_reason());
  (this->*handlers_[handler_index])(vp);
}

void vmexit_handler::invoke_termination() noexcept
{

}

//
// "Do-nothing" handlers for all VM-exits.
// VMX-instruction related VM-exits (VMREAD, VMWRITE, INVEPT, ...)
// have special fallback handler.  This comes in handy e.g. if you
// want them all to raise #UD.
//

void vmexit_handler::handle_exception_or_nmi(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_external_interrupt(vcpu_t& vp)                      noexcept { handle_fallback(vp); }
void vmexit_handler::handle_triple_fault(vcpu_t& vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_init_signal(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_startup_ipi(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_io_smi(vcpu_t& vp)                                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_smi(vcpu_t& vp)                                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_interrupt_window(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_nmi_window(vcpu_t& vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_task_switch(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_cpuid(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_getsec(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_hlt(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invd(vcpu_t& vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invlpg(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdpmc(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdtsc(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rsm_in_smm(vcpu_t& vp)                      noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_vmcall(vcpu_t& vp)                          noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmclear(vcpu_t& vp)                         noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmlaunch(vcpu_t& vp)                        noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrld(vcpu_t& vp)                         noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrst(vcpu_t& vp)                         noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmread(vcpu_t& vp)                          noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmresume(vcpu_t& vp)                        noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmwrite(vcpu_t& vp)                         noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxoff(vcpu_t& vp)                          noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxon(vcpu_t& vp)                           noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_mov_cr(vcpu_t& vp)                                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_mov_dr(vcpu_t& vp)                                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_io_instruction(vcpu_t& vp)                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdmsr(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_wrmsr(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_invalid_guest_state(vcpu_t& vp)               noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_msr_load(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_mwait(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_monitor_trap_flag(vcpu_t& vp)                       noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_monitor(vcpu_t& vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_pause(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_machine_check(vcpu_t& vp)                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_tpr_below_threshold(vcpu_t& vp)                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_apic_access(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_virtualized_eoi(vcpu_t& vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_gdtr_idtr_access(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ldtr_tr_access(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ept_violation(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ept_misconfiguration(vcpu_t& vp)                    noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invept(vcpu_t& vp)                          noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_rdtscp(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_vmx_preemption_timer_expired(vcpu_t& vp)            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invvpid(vcpu_t& vp)                         noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_wbinvd(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xsetbv(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_apic_write(vcpu_t& vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdrand(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invpcid(vcpu_t& vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_vmfunc(vcpu_t& vp)                          noexcept { handle_vm_fallback(vp); }
void vmexit_handler::handle_execute_encls(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdseed(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_page_modification_log_full(vcpu_t& vp)              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xsaves(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xrstors(vcpu_t& vp)                         noexcept { handle_fallback(vp); }

void vmexit_handler::handle_fallback(vcpu_t& vp)                                noexcept { (void)vp; }
void vmexit_handler::handle_vm_fallback(vcpu_t& vp)                             noexcept { (void)vp; }

}
