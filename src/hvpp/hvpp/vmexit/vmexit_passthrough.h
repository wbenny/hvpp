#pragma once
#include "hvpp/vmexit.h"

namespace hvpp {

//
// Base VM-exit handler.
//
// This handler tries to emulate what CPU normally does when trapped events
// and instructions occur.
//

class vmexit_passthrough_handler
  : public vmexit_handler
{
  public:
    auto setup(vcpu_t& vp) noexcept -> error_code_t override;
    void teardown(vcpu_t& vp) noexcept override;
    void terminate(vcpu_t& vp) noexcept override;

  protected:
    void handle_exception_or_nmi(vcpu_t& vp) noexcept override;
    void handle_external_interrupt(vcpu_t& vp) noexcept override;
    void handle_triple_fault(vcpu_t& vp) noexcept override;
    void handle_interrupt_window(vcpu_t& vp) noexcept override;
    void handle_nmi_window(vcpu_t& vp) noexcept override;
    void handle_execute_cpuid(vcpu_t& vp) noexcept override;
    void handle_execute_invd(vcpu_t& vp) noexcept override;
    void handle_execute_invlpg(vcpu_t& vp) noexcept override;
    void handle_execute_rdtsc(vcpu_t& vp) noexcept override;
    void handle_execute_vmcall(vcpu_t& vp) noexcept override;
    void handle_mov_cr(vcpu_t& vp) noexcept override;
    void handle_mov_dr(vcpu_t& vp) noexcept override;
    void handle_execute_io_instruction(vcpu_t& vp) noexcept override;
    void handle_execute_rdmsr(vcpu_t& vp) noexcept override;
    void handle_execute_wrmsr(vcpu_t& vp) noexcept override;
    void handle_gdtr_idtr_access(vcpu_t& vp) noexcept override;
    void handle_ldtr_tr_access(vcpu_t& vp) noexcept override;
    void handle_ept_violation(vcpu_t& vp) noexcept override;
    void handle_execute_rdtscp(vcpu_t& vp) noexcept override;
    void handle_execute_wbinvd(vcpu_t& vp) noexcept override;
    void handle_execute_xsetbv(vcpu_t& vp) noexcept override;
    void handle_execute_invpcid(vcpu_t& vp) noexcept override;

    //
    // VM-instructions.
    //
    void handle_execute_vmclear(vcpu_t& vp) noexcept override;
    void handle_execute_vmlaunch(vcpu_t& vp) noexcept override;
    void handle_execute_vmptrld(vcpu_t& vp) noexcept override;
    void handle_execute_vmptrst(vcpu_t& vp) noexcept override;
    void handle_execute_vmread(vcpu_t& vp) noexcept override;
    void handle_execute_vmresume(vcpu_t& vp) noexcept override;
    void handle_execute_vmwrite(vcpu_t& vp) noexcept override;
    void handle_execute_vmxoff(vcpu_t& vp) noexcept override;
    void handle_execute_vmxon(vcpu_t& vp) noexcept override;
    void handle_execute_invept(vcpu_t& vp) noexcept override;
    void handle_execute_invvpid(vcpu_t& vp) noexcept override;
    void handle_execute_vmfunc(vcpu_t& vp) noexcept override;

    void handle_vm_fallback(vcpu_t& vp) noexcept override;

    //
    //
    //
    virtual void handle_interrupt(vcpu_t& vp) noexcept;

    virtual void handle_emulate_syscall(vcpu_t& vp) noexcept;
    virtual void handle_emulate_sysret(vcpu_t& vp) noexcept;
    virtual void handle_emulate_rdtsc(vcpu_t& vp) noexcept;
    virtual void handle_emulate_rdtscp(vcpu_t& vp) noexcept;
};

}
