#pragma once
#include "ia32/arch.h"

#include "lib/error.h"

#include "vcpu.h"

#include <array>

namespace hvpp {

using namespace ia32;

template <
  typename T,
  size_t CPUID_0_MAX = 16,
  size_t CPUID_8_MAX = 16
>
struct vmexit_storage_t
{
  static constexpr size_t cpuid_0_max = CPUID_0_MAX;
  static constexpr size_t cpuid_8_max = CPUID_8_MAX;

  //
  // Storage for each VM-exit reason (ia32::vmx::exit_reason).
  // Currently the highest ID of exit reason is 65.
  //
  std::array<T, 65>           vmexit;

  //
  // Storage for each interrupt and exception vector (ia32::exception_vector).
  // This is subcategory of exit_reason::exception_or_nmi (0) and
  // exit_reason::external_interrupt (1).
  //
  std::array<T, 256>          exception_vector;

  //
  // Storage for each CPUID instruction:
  //   - cpuid_0:     CPUID with eax in range [ 0x0000'0000 - (0x0000'0000 + cpuid_0_max) ]
  //   - cpuid_8:     CPUID with eax in range [ 0x8000'0000 - (0x8000'0000 + cpuid_8_max) ]
  //   - cpuid_other: CPUID with other eax values
  // This is subcategory of exit_reason::execute_cpuid (10).
  //
  std::array<T, cpuid_0_max>  cpuid_0;
  std::array<T, cpuid_8_max>  cpuid_8;
  T                           cpuid_other;

  //
  // Storage for each MOV CR (from/to), CLTS and LMSW instruction.
  // Each array item in mov_from_cr/mov_to_cr represents counter for
  // specific CRn register.
  // This is subcategory of exit_reason::mov_cr (28).
  //
  std::array<T, 8>            mov_from_cr;
  std::array<T, 8>            mov_to_cr;
  T                           clts;
  T                           lmsw;

  //
  // Storage for each MOV DR (from/to) instruction.
  // Each array item in mov_from_dr/mov_to_dr represents storage for
  // specific DRn register.
  // This is subcategory of exit_reason::mov_dr (29).
  //
  std::array<T, 8>            mov_from_dr;
  std::array<T, 8>            mov_to_dr;

  //
  // Storage for each SGDT, SIDT, LGDT and LIDT instruction.
  // Each array item represents storage for specific instruction according
  // to the enum values defined in instruction_info_gdtr_idtr_access.
  // This is subcategory of exit_reason::gdtr_idtr_access (46).
  //
  std::array<T, 4>            gdtr_idtr;

  //
  // Storage for each SLDT, STR, LLDT and LTR instruction.
  // Each array item represents storage for specific instruction according
  // to the enum values defined in instruction_info_ldtr_tr_access.
  // This is subcategory of exit_reason::ldtr_tr_access (47).
  //
  std::array<T, 4>            ldtr_tr;

  //
  // Storage for each IN/OUT (INS/OUTS) instructions.
  // Each array item represents storage for specific I/O port.
  // This is subcategory of exit_reason::execute_io_instruction (30).
  //
  std::array<T, 0x10000>      io_in;
  std::array<T, 0x10000>      io_out;

  //
  // Storage for each RDMSR/WRMSR instructions.
  // Each array item represents storage for specific MSR number:
  //    rdmsr_0: MSR numbers in range 0x0000'0000 - 0x0000'1fff
  //    rdmsr_c: MSR numbers in range 0xc000'0000 - 0xc000'1fff
  //    rdmsr_other: MSR with other number values
  // The same applies with wrmsr_* members.
  // This is subcategory of exit_reason::execute_rdmsr (31)
  //                    and exit_reason::execute_wrmsr (32).
  //

  std::array<T, 0x2000>       rdmsr_0;
  std::array<T, 0x2000>       rdmsr_c;
  T                           rdmsr_other;

  std::array<T, 0x2000>       wrmsr_0;
  std::array<T, 0x2000>       wrmsr_c;
  T                           wrmsr_other;
};

//
// Abstract base class for VM-exit handlers.
//

class vmexit_handler
{
  public:
    //
    // Constructor & destructor.
    //
    // Note:
    //   Constructor & destructor is called in VMX-non-root mode.
    //   Therefore, avoid execution of any VMX instructions there.
    //
             vmexit_handler() noexcept;
    virtual ~vmexit_handler() noexcept;

    //
    // Non-copyable, non-movable.
    //
    vmexit_handler(const vmexit_handler& other) noexcept = delete;
    vmexit_handler(vmexit_handler&& other) noexcept = delete;
    vmexit_handler& operator=(const vmexit_handler& other) noexcept = delete;
    vmexit_handler& operator=(vmexit_handler&& other) noexcept = delete;

    //
    // This method is responsible for setup of VCPU state & VMCS and
    // allocation of resources for the VM-exit handler.
    //
    // It is called after VMXON but before VMLAUNCH, from the
    // vcpu_t::vmx_enter() method.
    //
    // Note:
    //   This method is called in VMX-root mode.
    //
    virtual auto setup(vcpu_t& vp) noexcept -> error_code_t;

    //
    // This method is responsible for freeing resources acquired in
    // the setup() method.
    //
    // It is called after VMXOFF, from the vcpu_t::vmx_leave() method.
    //
    // Note:
    //   This method is called in VMX-non-root mode.
    //
    virtual void teardown(vcpu_t& vp) noexcept;

    //
    // This method is responsible for initiating termination of the
    // VMX operation (i.e. leaving the virtualization mode).
    //
    // Note:
    //   It must NOT execute the VMXOFF instruction directly, because
    //   it is NOT called from VMX-root mode - instead, it should
    //   perform such operation that causes distinguishable VM-exit,
    //   which in turn executes the VMXOFF instruction.
    //
    //   Example might be performing VMCALL with RCX set to special
    //   value that handle_execute_vmcall() recognizes, and calls
    //   vp.vmx_leave().
    //
    //   See implementation of vmexit_passthrough_handler::terminate()
    //   and vmexit_passthrough_handler::handle_execute_vmcall().
    //
    virtual void terminate(vcpu_t& vp) noexcept;

    //
    // This method is called on every VM-exit.
    // By default this method delegates the execution control
    // to related VM-exit method (i.e.: for "execute CPUID VM-exit"
    // it calls handle_execute_cpuid() method).
    //
    // Note:
    //   Keep in mind that this method is not called for VM-exits
    //   that are not enabled in the VMCS.
    //
    virtual void handle(vcpu_t& vp) noexcept;

    //
    // This method is called in reaction to `vp.guest_resume()' call.
    // It should be responsible for cleaning up allocated resources,
    // that haven't been freed by object desctructors.
    //
    // Note:
    //   Use `auto _ = vp.stacked_lock_guard(lock)' for acquiring spinlocks.
    //   Spinlocks that are locked this way are by always unlocked.
    //
    virtual void handle_guest_resume(vcpu_t& vp, bool was_force_resumed) noexcept;

  protected:
    //
    // Separate handlers for each VM-exit reason.
    //
    virtual void handle_exception_or_nmi(vcpu_t& vp) noexcept;
    virtual void handle_external_interrupt(vcpu_t& vp) noexcept;
    virtual void handle_triple_fault(vcpu_t& vp) noexcept;
    virtual void handle_init_signal(vcpu_t& vp) noexcept;
    virtual void handle_startup_ipi(vcpu_t& vp) noexcept;
    virtual void handle_io_smi(vcpu_t& vp) noexcept;
    virtual void handle_smi(vcpu_t& vp) noexcept;
    virtual void handle_interrupt_window(vcpu_t& vp) noexcept;
    virtual void handle_nmi_window(vcpu_t& vp) noexcept;
    virtual void handle_task_switch(vcpu_t& vp) noexcept;
    virtual void handle_execute_cpuid(vcpu_t& vp) noexcept;
    virtual void handle_execute_getsec(vcpu_t& vp) noexcept;
    virtual void handle_execute_hlt(vcpu_t& vp) noexcept;
    virtual void handle_execute_invd(vcpu_t& vp) noexcept;
    virtual void handle_execute_invlpg(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdpmc(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdtsc(vcpu_t& vp) noexcept;
    virtual void handle_execute_rsm_in_smm(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmcall(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmclear(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmlaunch(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmptrld(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmptrst(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmread(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmresume(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmwrite(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmxoff(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmxon(vcpu_t& vp) noexcept;
    virtual void handle_mov_cr(vcpu_t& vp) noexcept;
    virtual void handle_mov_dr(vcpu_t& vp) noexcept;
    virtual void handle_execute_io_instruction(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdmsr(vcpu_t& vp) noexcept;
    virtual void handle_execute_wrmsr(vcpu_t& vp) noexcept;
    virtual void handle_error_invalid_guest_state(vcpu_t& vp) noexcept;
    virtual void handle_error_msr_load(vcpu_t& vp) noexcept;
    virtual void handle_execute_mwait(vcpu_t& vp) noexcept;
    virtual void handle_monitor_trap_flag(vcpu_t& vp) noexcept;
    virtual void handle_execute_monitor(vcpu_t& vp) noexcept;
    virtual void handle_execute_pause(vcpu_t& vp) noexcept;
    virtual void handle_error_machine_check(vcpu_t& vp) noexcept;
    virtual void handle_tpr_below_threshold(vcpu_t& vp) noexcept;
    virtual void handle_apic_access(vcpu_t& vp) noexcept;
    virtual void handle_virtualized_eoi(vcpu_t& vp) noexcept;
    virtual void handle_gdtr_idtr_access(vcpu_t& vp) noexcept;
    virtual void handle_ldtr_tr_access(vcpu_t& vp) noexcept;
    virtual void handle_ept_violation(vcpu_t& vp) noexcept;
    virtual void handle_ept_misconfiguration(vcpu_t& vp) noexcept;
    virtual void handle_execute_invept(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdtscp(vcpu_t& vp) noexcept;
    virtual void handle_vmx_preemption_timer_expired(vcpu_t& vp) noexcept;
    virtual void handle_execute_invvpid(vcpu_t& vp) noexcept;
    virtual void handle_execute_wbinvd(vcpu_t& vp) noexcept;
    virtual void handle_execute_xsetbv(vcpu_t& vp) noexcept;
    virtual void handle_apic_write(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdrand(vcpu_t& vp) noexcept;
    virtual void handle_execute_invpcid(vcpu_t& vp) noexcept;
    virtual void handle_execute_vmfunc(vcpu_t& vp) noexcept;
    virtual void handle_execute_encls(vcpu_t& vp) noexcept;
    virtual void handle_execute_rdseed(vcpu_t& vp) noexcept;
    virtual void handle_page_modification_log_full(vcpu_t& vp) noexcept;
    virtual void handle_execute_xsaves(vcpu_t& vp) noexcept;
    virtual void handle_execute_xrstors(vcpu_t& vp) noexcept;

    //
    // Fallback method.
    // Called by default by each non-overriden VM-exit handler,
    // except for VMX-instruction related handlers.
    //
    virtual void handle_fallback(vcpu_t& vp) noexcept;

    //
    // Fallback method.
    // Called by default by each non-overriden VMX-instruction
    // related VM-exit handler (i.e.: VMREAD, VMWRITE, INVEPT, ...).
    //
    virtual void handle_vm_fallback(vcpu_t& vp) noexcept;

  protected:
    using handler_fn_t = void (vmexit_handler::*)(vcpu_t&);
    const std::array<handler_fn_t, 65> handlers_;
};

//
// VM-exit compositor handler.
//

template <
  typename ...ARGS
>
class vmexit_compositor_handler
  : public vmexit_handler
{
  public:
    vmexit_compositor_handler() noexcept = default;
    ~vmexit_compositor_handler() noexcept override = default;

    auto setup(vcpu_t& vp) noexcept -> error_code_t override
    {
      for_each_element(handlers, [&](auto&& handler, int) {
        if (auto err = handler.setup(vp))
        { hvpp_assert(0); }
      });

      return {};
    }

    void teardown(vcpu_t& vp) noexcept override
    {
      for_each_element(handlers, [&](auto&& handler, int) {
        handler.teardown(vp);
      });
    }

    void terminate(vcpu_t& vp) noexcept override
    {
      for_each_element(handlers, [&](auto&& handler, int) {
        handler.terminate(vp);
        });
    }

    void handle(vcpu_t& vp) noexcept override
    {
      for_each_element(handlers, [&](auto&& handler, int) {
        handler.handle(vp);
      });
    }

    std::tuple<ARGS...> handlers;
};

}
