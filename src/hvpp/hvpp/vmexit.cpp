#include "vcpu.h"
#include "vmexit.h"

#include "ia32/vmx.h"
#include "ia32/interrupt.h"

namespace hvpp {

static const uint64_t terminate_vmcall_id = 0xDEAD;

vmexit_handler::vmexit_handler()
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

void vmexit_handler::handle(vcpu* vp) noexcept
{
  auto handler_index = static_cast<int>(vp->exit_reason());
  (this->*handlers_[handler_index])(vp);
}

void vmexit_handler::invoke_termination() noexcept
{
  vmx::vmcall(terminate_vmcall_id);
}

// void vmexit_handler::handle_exception_or_nmi(vcpu* vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_external_interrupt(vcpu* vp)                      noexcept { handle_fallback(vp); }
void vmexit_handler::handle_triple_fault(vcpu* vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_init_signal(vcpu* vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_startup_ipi(vcpu* vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_io_smi(vcpu* vp)                                  noexcept { handle_fallback(vp); }
void vmexit_handler::handle_smi(vcpu* vp)                                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_interrupt_window(vcpu* vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_nmi_window(vcpu* vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_task_switch(vcpu* vp)                             noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_cpuid(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_getsec(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_hlt(vcpu* vp)                             noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_invd(vcpu* vp)                            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invlpg(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdpmc(vcpu* vp)                           noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_rdtsc(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rsm_in_smm(vcpu* vp)                      noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_vmcall(vcpu* vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmclear(vcpu* vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmlaunch(vcpu* vp)                        noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrld(vcpu* vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmptrst(vcpu* vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmread(vcpu* vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmresume(vcpu* vp)                        noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmwrite(vcpu* vp)                         noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxoff(vcpu* vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_vmxon(vcpu* vp)                           noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_mov_cr(vcpu* vp)                                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_mov_dr(vcpu* vp)                                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_io_instruction(vcpu* vp)                  noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_rdmsr(vcpu* vp)                           noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_wrmsr(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_invalid_guest_state(vcpu* vp)               noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_msr_load(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_mwait(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_monitor_trap_flag(vcpu* vp)                       noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_monitor(vcpu* vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_pause(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_error_machine_check(vcpu* vp)                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_tpr_below_threshold(vcpu* vp)                     noexcept { handle_fallback(vp); }
void vmexit_handler::handle_apic_access(vcpu* vp)                             noexcept { handle_fallback(vp); }
void vmexit_handler::handle_virtualized_eoi(vcpu* vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_gdtr_idtr_access(vcpu* vp)                        noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ldtr_tr_access(vcpu* vp)                          noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_ept_violation(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_ept_misconfiguration(vcpu* vp)                    noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invept(vcpu* vp)                          noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_rdtscp(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_vmx_preemption_timer_expired(vcpu* vp)            noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invvpid(vcpu* vp)                         noexcept { handle_execute_vm_fallback(vp); }
// void vmexit_handler::handle_execute_wbinvd(vcpu* vp)                          noexcept { handle_fallback(vp); }
// void vmexit_handler::handle_execute_xsetbv(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_apic_write(vcpu* vp)                              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdrand(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_invpcid(vcpu* vp)                         noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_vmfunc(vcpu* vp)                          noexcept { handle_execute_vm_fallback(vp); }
void vmexit_handler::handle_execute_encls(vcpu* vp)                           noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_rdseed(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_page_modification_log_full(vcpu* vp)              noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xsaves(vcpu* vp)                          noexcept { handle_fallback(vp); }
void vmexit_handler::handle_execute_xrstors(vcpu* vp)                         noexcept { handle_fallback(vp); }

void vmexit_handler::handle_exception_or_nmi(vcpu* vp) noexcept
{
  auto interrupt = vp->exit_interrupt_info();

  switch (interrupt.type())
  {
    case vmx::interrupt_type::hardware_exception:
      switch (interrupt.vector())
      {
        case exception_vector::page_fault:
          write<cr2_t>(cr2_t{ vp->exit_qualification().pagefault.linear_address });
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

  vp->inject(interrupt);

  //
  // Do not increment rip by exit_instruction_length() in vcpu::entry_host().
  //
  // The RIP is controlled by entry_instruction_length() instead,
  // which is taken from interrupt_info::rip_adjust().
  //

  vp->suppress_rip_adjust();
}

void vmexit_handler::handle_execute_cpuid(vcpu* vp) noexcept
{
  int cpu_info[4];
  ia32_asm_cpuid_ex(cpu_info,
    static_cast<int>(vp->exit_context().rax),
    static_cast<int>(vp->exit_context().rcx));

  vp->exit_context().rax = cpu_info[0];
  vp->exit_context().rbx = cpu_info[1];
  vp->exit_context().rcx = cpu_info[2];
  vp->exit_context().rdx = cpu_info[3];
}

void vmexit_handler::handle_execute_invd(vcpu* vp) noexcept
{
  UNREFERENCED_PARAMETER(vp);

  ia32_asm_invd();
}

void vmexit_handler::handle_execute_rdtsc(vcpu* vp) noexcept
{
  auto tsc = ia32_asm_read_tsc();
  vp->exit_context().rax = tsc & 0xffffffff;
  vp->exit_context().rdx = tsc >> 32;
}

void vmexit_handler::handle_execute_vmcall(vcpu* vp) noexcept
{
  if (vp->exit_context().rcx == terminate_vmcall_id &&
      vp->guest_cs().selector.request_privilege_level == 0)
  {
    vp->terminate();
  }
  else
  {
    handle_execute_vm_fallback(vp);
  }
}

void vmexit_handler::handle_mov_cr(vcpu* vp) noexcept
{
  auto exit_qualification = vp->exit_qualification().mov_cr;
  uint64_t& gp_register = vp->exit_context().gp_register[exit_qualification.gp_register];

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_mov_cr::access_to_cr:
      switch (exit_qualification.cr_number)
      {
        case 0: vp->guest_cr0(cr0_t{ gp_register }); break;
        case 3: vp->guest_cr3(cr3_t{ gp_register });
                vmx::invvpid(vmx::invvpid_t::all_context); break;
        case 4: vp->guest_cr4(cr4_t{ gp_register }); break;
        default:
          break;
      }
      break;

    case vmx::exit_qualification_mov_cr::access_from_cr:
      switch (exit_qualification.cr_number)
      {
        case 0: gp_register = vp->guest_cr0().flags; break;
        case 3: gp_register = vp->guest_cr3().flags; break;
        case 4: gp_register = vp->guest_cr4().flags; break;
        default:
          break;
      }
      break;

    case vmx::exit_qualification_mov_cr::access_clts:
      ia32_asm_clear_ts();
      break;

    case vmx::exit_qualification_mov_cr::access_lmsw:
      ia32_asm_write_msw(static_cast<uint16_t>(exit_qualification.lmsw_source_data));
      break;

    default:
      break;
  }
}

void vmexit_handler::handle_mov_dr(vcpu* vp) noexcept
{
  auto exit_qualification = vp->exit_qualification().mov_dr;
  uint64_t& gp_register = vp->exit_context().gp_register[exit_qualification.gp_register];

  //
  // Do not allow manipulation with debug registers in > ring0.
  //

  if (vp->guest_ss().access.descriptor_privilege_level != 0)
  {
    __debugbreak();
    vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
    return;
  }

  //
  // Debug registers DR4 and DR5 are reserved when debug extensions are enabled (when the DE flag in control
  // register CR4 is set) and attempts to reference the DR4 and DR5 registers cause invalid - opcode exceptions(#UD).
  // When debug extensions are not enabled(when the DE flag is clear), these registers are aliased to debug registers
  // DR6 and DR7.
  // (ref: Vol3B[17.2.2(Debug Registers DR4 and DR5)])
  //

  if (vp->guest_cr4().debugging_extensions && (
      exit_qualification.dr_number == 4 ||
      exit_qualification.dr_number == 5))
  {
    __debugbreak();
    vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::invalid_opcode));
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

  if (vp->guest_dr7().general_detect)
  {
    __debugbreak();

    auto dr6 = read<dr6_t>();
    dr6.breakpoint_condition = 0;
    dr6.restricted_transactional_memory = true;
    dr6.debug_register_access_detected = true;

    write<dr6_t>(dr6);

    vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::debug));
    return;
  }

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_mov_dr::access_to_dr:
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
            vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
          }
          break;

        case 7:
          if ((gp_register >> 32) == 0)
          {
            vp->guest_dr7(dr7_t{ gp_register });
          }
          else
          {
            vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
          }
          break;

        default:
          break;
      }
      break;

    case vmx::exit_qualification_mov_dr::access_from_dr:
      switch (exit_qualification.dr_number)
      {
        case 0: gp_register = read<dr0_t>().flags; break;
        case 1: gp_register = read<dr1_t>().flags; break;
        case 2: gp_register = read<dr2_t>().flags; break;
        case 3: gp_register = read<dr3_t>().flags; break;
        case 4: gp_register = read<dr4_t>().flags; break;
        case 5: gp_register = read<dr5_t>().flags; break;
        case 6: gp_register = read<dr6_t>().flags; break;
        case 7: gp_register = vp->guest_dr7().flags; break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}

void vmexit_handler::handle_execute_io_instruction(vcpu* vp) noexcept
{
  auto exit_qualification = vp->exit_qualification().io_instruction;

  union
  {
    unsigned char*  as_byte_ptr;
    unsigned short* as_word_ptr;
    unsigned long*  as_dword_ptr;

    void*           as_ptr;
    uint64_t        as_uint64_t;
  } port_value;

  if (vp->guest_ss().access.descriptor_privilege_level != 0)
  {
    vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::general_protection));
    return;
  }

  //
  // Resolve address of the source or destination.
  //
  if (exit_qualification.string_instruction)
  {
    //
    // String operations always operate either on RDI (in) or RSI (out) registers.
    //
    port_value.as_ptr =
      exit_qualification.access_type == vmx::exit_qualification_io_instruction::access_in
        ? reinterpret_cast<void*>(vp->exit_context().rdi)
        : reinterpret_cast<void*>(vp->exit_context().rsi);
  }
  else
  {
    //
    // Save pointer to the RAX register.
    //
    port_value.as_ptr = &vp->exit_context().rax;
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
    ? static_cast<uint32_t>(vp->exit_context().rcx)
    : 1;

  uint32_t size = static_cast<uint32_t>(exit_qualification.size_of_access) + 1;
  //if (port == 0x5658 && !(!exit_qualification.string_instruction && size == 4 && exit_qualification.access_type == 0)) __debugbreak();

  auto guest_cr3 = vp->guest_cr3();
  auto hv_cr3 = read<cr3_t>();
  write(guest_cr3);

//   if (port == 0x5658)
//   hv_log("%s 0x%04x (%u bytes)%s",
//     (exit_qualification.access_type == vmx::exit_qualification_io_instruction::access_in)
//       ? "in from port"
//       : "out to port",
//     port, count * size, (exit_qualification.string_instruction)
//       ? "(string)" : "");
//
  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_io_instruction::access_in:
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
        // Note that port_value holds pointer to the vp->exit_context().rax member,
        // therefore we're directly overwriting the RAX value.
        //
        switch (size)
        {
          case 1: *port_value.as_byte_ptr = ia32_asm_in_byte(port); break;
          case 2: *port_value.as_word_ptr = ia32_asm_in_word(port); break;
          case 4:
            ia32_asm_in_dword_ex(port, &vp->exit_context().rax, &vp->exit_context().rbx, &vp->exit_context().rcx, &vp->exit_context().rdx);

            //*port_value.as_dword_ptr = ia32_asm_in_dword(port);
            break;
        }
      }
    break;

    case vmx::exit_qualification_io_instruction::access_out:
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
        // Note that port_value holds pointer to the vp->exit_context().rax member,
        // therefore we're directly reading from the RAX value.
        //
        switch (size)
        {
          case 1: ia32_asm_out_byte(port, *port_value.as_byte_ptr); break;
          case 2: ia32_asm_out_word(port, *port_value.as_word_ptr); break;
          case 4:
            ia32_asm_out_dword_ex(port, &vp->exit_context().rax, &vp->exit_context().rbx, &vp->exit_context().rcx, &vp->exit_context().rdx);

            //ia32_asm_out_dword(port, *port_value.as_dword_ptr);
            break;
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
      exit_qualification.access_type == vmx::exit_qualification_io_instruction::access_in
        ? vp->exit_context().rdi
        : vp->exit_context().rsi;

    if (vp->exit_context().rflags.direction_flag)
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
      vp->exit_context().rcx = 0;
    }
  }
  write(hv_cr3);
}

void vmexit_handler::handle_execute_rdmsr(vcpu* vp) noexcept
{
  uint32_t msr_id    = static_cast<uint32_t>(vp->exit_context().rcx);
  uint64_t msr_value = msr::read(msr_id);

  vp->exit_context().rax = msr_value & 0xffffffff;
  vp->exit_context().rdx = msr_value >> 32;
}

void vmexit_handler::handle_execute_wrmsr(vcpu* vp) noexcept
{
  uint32_t msr_id    = static_cast<uint32_t>(vp->exit_context().rcx);
  uint64_t msr_value =
    vp->exit_context().rax |
    vp->exit_context().rdx >> 32;

  msr::write(msr_id, msr_value);
}

void vmexit_handler::handle_ept_violation(vcpu* vp) noexcept
{
  //
  // TODO
  //
  handle_fallback(vp);
}

void vmexit_handler::handle_execute_rdtscp(vcpu* vp) noexcept
{
  uint32_t tsc_aux;
  auto tsc = ia32_asm_read_tscp(&tsc_aux);
  vp->exit_context().rax = tsc & 0xffffffff;
  vp->exit_context().rdx = tsc >> 32;
  vp->exit_context().rcx = tsc_aux;
}

void vmexit_handler::handle_execute_wbinvd(vcpu* vp) noexcept
{
  UNREFERENCED_PARAMETER(vp);

  ia32_asm_wb_invd();
}

void vmexit_handler::handle_execute_xsetbv(vcpu* vp) noexcept
{
  ia32_asm_write_xcr(
    static_cast<uint32_t>(vp->exit_context().rcx),
    static_cast<uint64_t>(vp->exit_context().rdx << 32 | vp->exit_context().rax));
}

void vmexit_handler::handle_execute_vm_fallback(vcpu* vp) noexcept
{
  vp->inject(interrupt_info(vmx::interrupt_type::hardware_exception, exception_vector::invalid_opcode));
}

}
