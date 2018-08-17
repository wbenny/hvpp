#include "vmexit.h"
#include "vcpu.h"
#include "config.h"

#include "ia32/vmx.h"

#include "lib/assert.h"
#include "lib/cr3_guard.h"

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND
# include "lib/vmware/vmware.h"
#endif

namespace hvpp {

static constexpr uint64_t vmcall_terminate_id  = 0xDEAD;
static constexpr uint64_t vmcall_breakpoint_id = 0xAABB;

vmexit_handler::vmexit_handler() noexcept
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

void vmexit_handler::setup(vcpu_t& vp) noexcept
{
  //
  // Setup guest VMCS state to bare minimum. This setup mirrors current state
  // of the OS.
  //
  vp.cr0_shadow(read<cr0_t>());
  vp.cr4_shadow(read<cr4_t>());

  vp.guest_cr0(read<cr0_t>());
  vp.guest_cr3(read<cr3_t>());
  vp.guest_cr4(read<cr4_t>());

  vp.guest_debugctl(msr::read<msr::debugctl_t>());
  vp.guest_dr7(read<dr7_t>());
  vp.guest_rflags(read<rflags_t>());

  auto gdtr = read<gdtr_t>();
  vp.guest_gdtr(gdtr);
  vp.guest_idtr(read<idtr_t>());
  vp.guest_cs(seg_t{ gdtr, read<cs_t>() });
  vp.guest_ds(seg_t{ gdtr, read<ds_t>() });
  vp.guest_es(seg_t{ gdtr, read<es_t>() });
  vp.guest_fs(seg_t{ gdtr, read<fs_t>() });
  vp.guest_gs(seg_t{ gdtr, read<gs_t>() });
  vp.guest_ss(seg_t{ gdtr, read<ss_t>() });
  vp.guest_tr(seg_t{ gdtr, read<tr_t>() });
  vp.guest_ldtr(seg_t{ gdtr, read<ldtr_t>() });
}

void vmexit_handler::handle(vcpu_t& vp) noexcept
{
  auto handler_index = static_cast<int>(vp.exit_reason());
  (this->*handlers_[handler_index])(vp);
}

void vmexit_handler::invoke_termination() noexcept
{
  vmx::vmcall(vmcall_terminate_id);
}

// void vmexit_handler::handle_exception_or_nmi(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_external_interrupt(vcpu_t& vp)                      noexcept { handle_fallback(vp); }
//void vmexit_handler::handle_triple_fault(vcpu_t& vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_init_signal(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_startup_ipi(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_io_smi(vcpu_t& vp)                                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_smi(vcpu_t& vp)                                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_interrupt_window(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_nmi_window(vcpu_t& vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_task_switch(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_cpuid(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_getsec(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_hlt(vcpu_t& vp)                             noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_invd(vcpu_t& vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invlpg(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdpmc(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_rdtsc(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rsm_in_smm(vcpu_t& vp)                      noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_vmcall(vcpu_t& vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmclear(vcpu_t& vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmlaunch(vcpu_t& vp)                        noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrld(vcpu_t& vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrst(vcpu_t& vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmread(vcpu_t& vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmresume(vcpu_t& vp)                        noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmwrite(vcpu_t& vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxoff(vcpu_t& vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxon(vcpu_t& vp)                           noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_mov_cr(vcpu_t& vp)                                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_mov_dr(vcpu_t& vp)                                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_io_instruction(vcpu_t& vp)                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_rdmsr(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_wrmsr(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
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
//void vmexit_handler::handle_gdtr_idtr_access(vcpu_t& vp)                        noexcept { handle_fallback(vp); }
//void vmexit_handler::handle_ldtr_tr_access(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_ept_violation(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ept_misconfiguration(vcpu_t& vp)                    noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invept(vcpu_t& vp)                          noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_rdtscp(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_vmx_preemption_timer_expired(vcpu_t& vp)            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invvpid(vcpu_t& vp)                         noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_wbinvd(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_xsetbv(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_apic_write(vcpu_t& vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdrand(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invpcid(vcpu_t& vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_vmfunc(vcpu_t& vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_encls(vcpu_t& vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdseed(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_page_modification_log_full(vcpu_t& vp)              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xsaves(vcpu_t& vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xrstors(vcpu_t& vp)                         noexcept { handle_fallback(vp); }

void vmexit_handler::handle_exception_or_nmi(vcpu_t& vp) noexcept
{
  auto interrupt = vp.exit_interrupt_info();

  switch (interrupt.type())
  {
    case vmx::interrupt_type::hardware_exception:
      switch (interrupt.vector())
      {
        case exception_vector::general_protection:

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND

        {
            //
            // VMWare I/O backdoor (port 0x5658/0x5659) workaround.
            //
            cr3_guard _(vp.guest_cr3());

            vmx::exit_qualification_io_instruction_t exit_qualification;
            if (try_decode_io_instruction(vp.exit_context(), exit_qualification))
            {
              ia32_asm_io_with_context(exit_qualification, vp.exit_context());
              return;
            }
          }

#endif

          break;

        case exception_vector::page_fault:
          write<cr2_t>(cr2_t{ vp.exit_qualification().linear_address });
          break;

        default:
          break;
      }
      break;

    case vmx::interrupt_type::software_exception:
      switch (interrupt.vector())
      {
        case exception_vector::breakpoint:
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }

  //
  // Just reinject the event.
  //
  vp.inject(interrupt);

  //
  // Do not increment rip by exit_instruction_length() in vcpu::entry_host().
  //
  // The RIP is controlled by entry_instruction_length() instead,
  // which is taken from interrupt_info::rip_adjust().
  //
  vp.suppress_rip_adjust();
}

void vmexit_handler::handle_triple_fault(vcpu_t& vp) noexcept
{
  (void)(vp);

  //
  // Hang.
  //
  for (;;)
  {
    ia32_asm_pause();
    ia32_asm_halt();
  }
}

void vmexit_handler::handle_execute_cpuid(vcpu_t& vp) noexcept
{
  int cpu_info[4];
  ia32_asm_cpuid_ex(cpu_info,
                    vp.exit_context().eax,
                    vp.exit_context().ecx);

  vp.exit_context().rax = cpu_info[0];
  vp.exit_context().rbx = cpu_info[1];
  vp.exit_context().rcx = cpu_info[2];
  vp.exit_context().rdx = cpu_info[3];
}

void vmexit_handler::handle_execute_invd(vcpu_t& vp) noexcept
{
  (void)(vp);

  ia32_asm_invd();
}

void vmexit_handler::handle_execute_rdtsc(vcpu_t& vp) noexcept
{
  uint64_t tsc = ia32_asm_read_tsc();
  vp.exit_context().rax = tsc & 0xffffffff;
  vp.exit_context().rdx = tsc >> 32;
}

void vmexit_handler::handle_execute_vmcall(vcpu_t& vp) noexcept
{
  if (vp.exit_context().rcx == vmcall_terminate_id &&
      vp.guest_cs().selector.request_privilege_level == 0)
  {
    vp.terminate();
  }
  else if (vp.exit_context().rcx == vmcall_breakpoint_id)
  {
    __debugbreak();
  }
  else
  {
    handle_execute_vm_fallback(vp);
  }
}

void vmexit_handler::handle_mov_cr(vcpu_t& vp) noexcept
{
  auto exit_qualification = vp.exit_qualification().mov_cr;
  uint64_t& gp_register = vp.exit_context().gp_register[exit_qualification.gp_register];

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_mov_cr_t::access_to_cr:
      switch (exit_qualification.cr_number)
      {
        case 0:
          vp.guest_cr0(cr0_t{ gp_register });
          vp.cr0_shadow(cr0_t{ gp_register });
          break;

        case 3:
          {
            //
            // If CR4.PCIDE = 1, bit 63 of the source operand to MOV to CR3 determines whether the instruction invalidates
            // entries in the TLBs and the paging-structure caches. The instruction does not modify bit 63 of CR3, which is
            // reserved and always 0.
            // (ref: Vol2B(MOV-Move to/from Control Registers)
            // (see: Vol3A[4.10.4.1(Operations that Invalidate TLBs and Paging-Structure Caches)]
            //
                auto cr3 = cr3_t{ gp_register };
                if (vp.guest_cr4().pcid_enable)
                {
                  //
                  // Equivalent to:
                  //   gp_register &= ~(1ull << 63);
                  //
                  cr3.pcid_invalidate = false;
                }
                vp.guest_cr3(cr3);
                vmx::invvpid(vmx::invvpid_t::all_context);
            }
          break;

        case 4:
          vp.guest_cr4(cr4_t{ gp_register });
          vp.cr4_shadow(cr4_t{ gp_register });
          break;

        case 8:
          /* unimplemented */
          break;
      }
      break;

    case vmx::exit_qualification_mov_cr_t::access_from_cr:
      switch (exit_qualification.cr_number)
      {
        case 3: gp_register = vp.guest_cr3().flags; break;
        case 8: /* unimplemented */ break;
      }
      break;

    case vmx::exit_qualification_mov_cr_t::access_clts:
      {
        auto cr0 = vp.guest_cr0();
        cr0.task_switched = false;
        vp.guest_cr0(cr0);
        vp.cr0_shadow(cr0);
      }
      break;

    case vmx::exit_qualification_mov_cr_t::access_lmsw:
      {
        auto msw = static_cast<uint16_t>(exit_qualification.lmsw_source_data);
        auto cr0 = vp.guest_cr0();

        //
        // Loads the source operand into the machine status word, bits 0 through 15 of register CR0. The source operand can
        // be a 16-bit general-purpose register or a memory location. Only the low-order 4 bits of the source operand (which
        // contains the PE, MP, EM, and TS flags) are loaded into CR0. The PG, CD, NW, AM, WP, NE, and ET flags of CR0 are
        // not affected. The operand-size attribute has no effect on this instruction.
        // If the PE flag of the source operand (bit 0) is set to 1, the instruction causes the processor to switch to protected
        // mode. While in protected mode, the LMSW instruction cannot be used to clear the PE flag and force a switch back
        // to real-address mode.
        // (ref: Vol2A[(LMSW—Load Machine Status Word)])
        //
        // TL;DR:
        //   CR0[0:3] <- SRC[0:3];
        //
        //   ...except if CR0.PE (bit 0) is already 1 - then do not change that bit (lmsw can't be used to switch back to
        //   real mode from the protected mode.
        //

        cr0.flags &=      ~0b1110;
        cr0.flags |= msw & 0b1111;

        vp.guest_cr0(cr0);
        vp.cr0_shadow(cr0);
      }
      break;
  }
}

void vmexit_handler::handle_mov_dr(vcpu_t& vp) noexcept
{
  auto exit_qualification = vp.exit_qualification().mov_dr;
  uint64_t& gp_register = vp.exit_context().gp_register[exit_qualification.gp_register];

  //
  // The MOV DR instruction causes a VM exit if the "MOV-DR exiting" VM-execution
  // control is 1. Such VM exits represent an exception to the principles identified
  // in Section 25.1.1 (Relative Priority of Faults and VM Exits) in that they take
  // priority over the following: general-protection exceptions based on privilege
  // level; and invalid - opcode exceptions that occur because CR4.DE = 1 and the
  // instruction specified access to DR4 or DR5.
  // (ref: Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)])
  //
  // In another words: CPU usually prioritizes exceptions. For example RDMSR executed
  // at CPL = 3 won't cause VM-exit - it causes #GP instead. MOV DR is exception to
  // this rule, as it ALWAYS cause VM-exit.
  //
  // Normally, CPU allows you to write to DR registers only at CPL = 0, otherwise it
  // causes #GP. Therefore we'll simulate the exact same behavior here.
  //

  if (vp.guest_cs().access.descriptor_privilege_level != 0)
  {
    __debugbreak();
    vp.inject(interrupt_info_t(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
    return;
  }

  //
  // Debug registers DR4 and DR5 are reserved when debug extensions are enabled (when the DE flag in control
  // register CR4 is set) and attempts to reference the DR4 and DR5 registers cause invalid - opcode exceptions(#UD).
  // When debug extensions are not enabled(when the DE flag is clear), these registers are aliased to debug registers
  // DR6 and DR7.
  // (ref: Vol3B[17.2.2(Debug Registers DR4 and DR5)])
  //

  if (vp.guest_cr4().debugging_extensions && (
      exit_qualification.dr_number == 4 ||
      exit_qualification.dr_number == 5))
  {
    __debugbreak();
    vp.inject(interrupt_info_t(vmx::interrupt_type::hardware_exception, exception_vector::invalid_opcode));
    return;
  }

  //
  // Blatantly copied from ksm. This is what Intel Manual says:
  //
  // Enables (when set) debug-register protection, which causes a
  // debug exception to be generated prior to any MOV instruction that accesses a debug register.When such a
  // condition is detected, the BD flag in debug status register DR6 is set prior to generating the exception.This
  // condition is provided to support in - circuit emulators.
  // When the emulator needs to access the debug registers, emulator software can set the GD flag to prevent
  // interference from the program currently executing on the processor.
  // The processor clears the GD flag upon entering to the debug exception handler, to allow the handler access to
  // the debug registers.
  // (ref: Vol3B[17.2.4(Debug Control Register (DR7)])
  //

  if (vp.guest_dr7().general_detect)
  {
    __debugbreak();

    auto dr6 = read<dr6_t>();
    dr6.breakpoint_condition = 0;
    dr6.restricted_transactional_memory = true;
    dr6.debug_register_access_detected = true;

    write<dr6_t>(dr6);

    vp.inject(interrupt_info_t(vmx::interrupt_type::hardware_exception, exception_vector::debug));
    return;
  }

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_mov_dr_t::access_to_dr:
      switch (exit_qualification.dr_number)
      {
        case 0: write<dr0_t>(dr0_t{ gp_register }); break;
        case 1: write<dr1_t>(dr1_t{ gp_register }); break;
        case 2: write<dr2_t>(dr2_t{ gp_register }); break;
        case 3: write<dr3_t>(dr3_t{ gp_register }); break;
        case 4: write<dr4_t>(dr4_t{ gp_register }); break;
        case 5: write<dr5_t>(dr5_t{ gp_register }); break;

        //
        // In 64-bit mode, the upper 32 bits of DR6 and DR7 are reserved and must be written with zeros. Writing 1 to any of
        // the upper 32 bits results in a #GP(0) exception.
        // (ref: Vol3B[17.2.6(Debug Registers and Intel® 64 Processors)])
        //

        case 6:
          if ((gp_register >> 32) == 0)
          {
            write<dr6_t>(dr6_t{ gp_register });
          }
          else
          {
            vp.inject(interrupt_info_t(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
          }
          break;

        case 7:
          if ((gp_register >> 32) == 0)
          {
            vp.guest_dr7(dr7_t{ gp_register });
          }
          else
          {
            vp.inject(interrupt_info_t(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
          }
          break;

        default:
          break;
      }
      break;

    case vmx::exit_qualification_mov_dr_t::access_from_dr:
      switch (exit_qualification.dr_number)
      {
        case 0: gp_register = read<dr0_t>().flags; break;
        case 1: gp_register = read<dr1_t>().flags; break;
        case 2: gp_register = read<dr2_t>().flags; break;
        case 3: gp_register = read<dr3_t>().flags; break;
        case 4: gp_register = read<dr4_t>().flags; break;
        case 5: gp_register = read<dr5_t>().flags; break;
        case 6: gp_register = read<dr6_t>().flags; break;
        case 7: gp_register = vp.guest_dr7().flags; break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}

void vmexit_handler::handle_execute_io_instruction(vcpu_t& vp) noexcept
{
  auto exit_qualification = vp.exit_qualification().io_instruction;

  union
  {
    unsigned char*  as_byte_ptr;
    unsigned short* as_word_ptr;
    unsigned long*  as_dword_ptr;

    void*           as_ptr;
    uint64_t        as_uint64_t;
  } port_value;

  //
  // We don't check if CPL == 0 here, because the CPU would raise #GP instead
  // of VM-exit.
  //
  // See Vol3C[25.1.1(Relative Priority of Faults and VM Exits)]
  //

  //
  // Resolve address of the source or destination.
  //
  if (exit_qualification.string_instruction)
  {
    //
    // String operations always operate either on RDI (in) or RSI (out) registers.
    //
    port_value.as_ptr =
      exit_qualification.access_type == vmx::exit_qualification_io_instruction_t::access_in
        ? vp.exit_context().rdi_as_pointer
        : vp.exit_context().rsi_as_pointer;
  }
  else
  {
    //
    // Save pointer to the RAX register.
    //
    port_value.as_ptr = &vp.exit_context().rax;
  }

  //
  // Resolve port as a nice 16bit number.
  //
  uint16_t port = static_cast<uint16_t>(exit_qualification.port_number);

  //
  // Resolve number of bytes to send/receive. REP prefixed instructions always
  // take their count from *CX register.
  //
  uint32_t count = exit_qualification.rep_prefixed
    ? vp.exit_context().ecx
    : 1;

  uint32_t size = static_cast<uint32_t>(exit_qualification.size_of_access) + 1;

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND

  (void)(port);
  (void)(count);
  (void)(size);

  ia32_asm_io_with_context(exit_qualification, vp.exit_context());

#else

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_io_instruction_t::access_in:
      if (exit_qualification.string_instruction)
      {
        switch (size)
        {
          case 1: ia32_asm_in_byte_string(port, port_value.as_byte_ptr, count); break;
          case 2: ia32_asm_in_word_string(port, port_value.as_word_ptr, count); break;
          case 4: ia32_asm_in_dword_string(port, port_value.as_dword_ptr, count); break;
        }
      }
      else
      {
        //
        // Note that port_value holds pointer to the vp.exit_context().rax member,
        // therefore we're directly overwriting the RAX value.
        //
        switch (size)
        {
          case 1: *port_value.as_byte_ptr = ia32_asm_in_byte(port); break;
          case 2: *port_value.as_word_ptr = ia32_asm_in_word(port); break;
          case 4: *port_value.as_dword_ptr = ia32_asm_in_dword(port); break;
        }
      }
    break;

    case vmx::exit_qualification_io_instruction_t::access_out:
      if (exit_qualification.string_instruction)
      {
        switch (size)
        {
          case 1: ia32_asm_out_byte_string(port, port_value.as_byte_ptr, count); break;
          case 2: ia32_asm_out_word_string(port, port_value.as_word_ptr, count); break;
          case 4: ia32_asm_out_dword_string(port, port_value.as_dword_ptr, count); break;
        }
      }
      else
      {
        //
        // Note that port_value holds pointer to the vp.exit_context().rax member,
        // therefore we're directly reading from the RAX value.
        //
        switch (size)
        {
          case 1: ia32_asm_out_byte(port, *port_value.as_byte_ptr); break;
          case 2: ia32_asm_out_word(port, *port_value.as_word_ptr); break;
          case 4: ia32_asm_out_dword(port, *port_value.as_dword_ptr); break;
        }
      }
      break;
  }

  if (exit_qualification.string_instruction)
  {
    //
    // Update register:
    // If the DF (direction flag) is set, decrement, otherwise increment.
    //
    // For in the register is RDI, for out it's RSI.
    //
    uintptr_t& gp_register =
      exit_qualification.access_type == vmx::exit_qualification_io_instruction_t::access_in
        ? vp.exit_context().rdi
        : vp.exit_context().rsi;

    if (vp.exit_context().rflags.direction_flag)
    {
      gp_register -= count * size;
    }
    else
    {
      gp_register += count * size;
    }

    //
    // We've sent/received everything, reset counter register to 0.
    //
    if (exit_qualification.rep_prefixed)
    {
      vp.exit_context().rcx = 0;
    }
  }

#endif
}

void vmexit_handler::handle_execute_rdmsr(vcpu_t& vp) noexcept
{
  uint32_t msr_id = vp.exit_context().ecx;
  uint64_t msr_value;

  switch (msr_id)
  {
    case msr::debugctl_t::msr_id:
      msr_value = vp.guest_debugctl().flags;
      break;

    case msr::fs_base_t::msr_id:
      msr_value = reinterpret_cast<uint64_t>(vp.guest_segment_base_address(context_t::seg_fs));
      break;

    case msr::gs_base_t::msr_id:
      msr_value = reinterpret_cast<uint64_t>(vp.guest_segment_base_address(context_t::seg_gs));
      break;

    default:
      msr_value = msr::read(msr_id);
      break;
  }

  vp.exit_context().rax = msr_value & 0xffffffff;
  vp.exit_context().rdx = msr_value >> 32;
}

void vmexit_handler::handle_execute_wrmsr(vcpu_t& vp) noexcept
{
  uint32_t msr_id    = vp.exit_context().ecx;
  uint64_t msr_value =
    vp.exit_context().rax |
    vp.exit_context().rdx << 32;

  switch (msr_id)
  {
    case msr::debugctl_t::msr_id:
      vp.guest_debugctl(msr::debugctl_t{ msr_value });
      break;

    case msr::fs_base_t::msr_id:
      vp.guest_segment_base_address(context_t::seg_fs, reinterpret_cast<void*>(msr_value));
      break;

    case msr::gs_base_t::msr_id:
      vp.guest_segment_base_address(context_t::seg_gs, reinterpret_cast<void*>(msr_value));
      break;

    default:
      msr::write(msr_id, msr_value);
      break;
  }
}

void vmexit_handler::handle_gdtr_idtr_access(vcpu_t& vp) noexcept
{
  auto instruction_info = vp.exit_instruction_info().gdtr_idtr_access;
  void* guest_va = vp.exit_instruction_info_guest_va();

  union
  {
    gdtr_t gdtr;
    idtr_t idtr;
  };

  cr3_guard _(vp.guest_cr3());

  switch (instruction_info.instruction)
  {
    case vmx::instruction_info_gdtr_idtr_access_t::instruction_sgdt:
      gdtr = vp.guest_gdtr();
      memcpy(guest_va, &gdtr, sizeof(gdtr));
      break;

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_sidt:
      idtr = vp.guest_idtr();
      memcpy(guest_va, &idtr, sizeof(idtr));
      break;

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_lgdt:
      memcpy(&gdtr, guest_va, sizeof(gdtr));
      vp.guest_gdtr(gdtr);
      break;

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_lidt:
      memcpy(&idtr, guest_va, sizeof(idtr));
      vp.guest_idtr(idtr);
      break;
  }
}

void vmexit_handler::handle_ldtr_tr_access(vcpu_t& vp) noexcept
{
  auto instruction_info = vp.exit_instruction_info().ldtr_tr_access;

  cr3_guard _(vp.guest_cr3());

  uint16_t& low_word =
    instruction_info.access_type == vmx::instruction_info_t::access_memory
      ? *reinterpret_cast<uint16_t*>(vp.exit_instruction_info_guest_va())
      // instruction_info.access_type == vmx::instruction_info::access_register
      :  reinterpret_cast<uint16_t&>(vp.exit_context().gp_register[instruction_info.register_1]);

  switch (instruction_info.instruction)
  {
    case vmx::instruction_info_ldtr_tr_access_t::instruction_sldt:
      low_word = vp.guest_segment_selector(context_t::seg_ldtr).flags;
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_str:
      low_word = vp.guest_segment_selector(context_t::seg_tr).flags;
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_lldt:
      vp.guest_segment_selector(context_t::seg_ldtr, seg_selector_t{ low_word });
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_ltr:
      {
        //
        // After the segment selector is loaded in the task register, the processor uses the segment selector to locate the
        // segment descriptor for the TSS in the global descriptor table(GDT). It then loads the segment limit and base
        // address for the TSS from the segment descriptor into the task register.The task pointed to by the task register is
        // marked busy, but a switch to the task does not occur.
        // (ref: Vol2A[(LTR-Load Task Register)])
        //
        // TL;DR:
        //   LTR instruction sets busy bit in the TSS and we need to emulate this behavior.
        //
        auto selector = seg_selector_t{ low_word };
        vp.guest_segment_selector(context_t::seg_tr, selector);

        auto& descriptor = vp.guest_gdtr()[selector];
        descriptor.access.type |= seg_access_t::type_tss_busy_flag;
      }
      break;
  }
}

void vmexit_handler::handle_ept_violation(vcpu_t& vp) noexcept
{
  //
  // TODO
  //
  handle_fallback(vp);
}

void vmexit_handler::handle_execute_rdtscp(vcpu_t& vp) noexcept
{
  uint32_t tsc_aux;
  uint64_t tsc = ia32_asm_read_tscp(&tsc_aux);

  vp.exit_context().rax = tsc & 0xffffffff;
  vp.exit_context().rdx = tsc >> 32;
  vp.exit_context().rcx = tsc_aux;
}

void vmexit_handler::handle_execute_wbinvd(vcpu_t& vp) noexcept
{
  (void)(vp);

  ia32_asm_wb_invd();
}

void vmexit_handler::handle_execute_xsetbv(vcpu_t& vp) noexcept
{
  ia32_asm_write_xcr(vp.exit_context().ecx,
                     vp.exit_context().rdx << 32 |
                     vp.exit_context().rax);
}

void vmexit_handler::handle_execute_vm_fallback(vcpu_t& vp) noexcept
{
  vp.inject(
    interrupt_info_t(vmx::interrupt_type::hardware_exception,
                     exception_vector::invalid_opcode));
}

}
