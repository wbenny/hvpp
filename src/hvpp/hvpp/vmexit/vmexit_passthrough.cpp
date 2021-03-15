#include "vmexit_passthrough.h"

#include "hvpp/config.h"
#include "hvpp/vcpu.h"

#include "hvpp/lib/assert.h"
#include "hvpp/lib/log.h"
#include "hvpp/lib/cr3_guard.h"
#include "hvpp/lib/debugger.h"

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND
# include "hvpp/lib/vmware/vmware.h"
#endif

#include <algorithm>

namespace hvpp {

static constexpr uint64_t vmcall_terminate_id  = 0xDEAD;
static constexpr uint64_t vmcall_breakpoint_id = 0xAABB;

namespace detail
{
  static const uint8_t* skip_prefixes(const uint8_t* first, const uint8_t* last) noexcept
  {
	  //
	  // Return the first byte of the opcode that is not a prefix.
	  //
	  return std::find_if(first, last, [&](uint8_t byte){		  
		  //
		  // List of prefix types that should be skipped, LOCK is not included as it'd #UD.
		  //
		  static constexpr uint8_t skip_table[] = {
			  0xF2, 0xF3, 0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65, 0x2E, 0x3E, 0x66, 0x67
		  };
		  return std::find(std::begin(skip_table), std::end(skip_table), byte) == std::end(skip_table);
	  }
  }
  
  static bool is_syscall_instruction(const uint8_t* first, const uint8_t* last) noexcept
  {
    static constexpr uint8_t opcode[] = { 0x0f, 0x05 };
    first = skip_prefixes(first, last);
    return (last - first) >= std::size(opcode) && memcmp(first, opcode, sizeof(opcode)) == 0;
  }

  static bool is_sysret_instruction(const uint8_t* first, const uint8_t* last) noexcept
  {
    static constexpr uint8_t opcode[] = { 0x48, 0x0f, 0x07 };
    first = skip_prefixes(first, last);
    return (last-first) >= std::size(opcode) && memcmp(first, opcode, sizeof(opcode)) == 0;
  }

  static bool is_rdtsc_instruction(const uint8_t* first, const uint8_t* last) noexcept
  {
    static constexpr uint8_t opcode[] = { 0x0f, 0x31 };
    first = skip_prefixes(first, last);
    return (last - first) >= std::size(opcode) && memcmp(first, opcode, sizeof(opcode)) == 0;
  }

  static bool is_rdtscp_instruction(const uint8_t* first, const uint8_t* last) noexcept
  {
    static constexpr uint8_t opcode[] = { 0x0f, 0x01, 0xf9 };
    first = skip_prefixes(first, last);
    return (last - first) >= std::size(opcode) && memcmp(first, opcode, sizeof(opcode)) == 0;
  }
}

auto vmexit_passthrough_handler::setup(vcpu_t& vp) noexcept -> error_code_t
{
  //
  // Setup guest VMCS state to bare minimum.
  // This setup mirrors current state of the OS.
  //
  vp.cr0_shadow(read<cr0_t>());
  vp.cr4_shadow(read<cr4_t>());

  vp.guest_cr0(read<cr0_t>());
  vp.guest_cr3(read<cr3_t>());
  vp.guest_cr4(read<cr4_t>());

  vp.guest_debugctl(msr::read<msr::debugctl_t>());
  vp.guest_dr7(read<dr7_t>());
  vp.guest_rflags(read<rflags_t>());

  const auto gdtr = read<gdtr_t>();
  const auto idtr = read<idtr_t>();

  vp.guest_gdtr(gdtr);
  vp.guest_idtr(idtr);
  vp.guest_cs(segment_t{ gdtr, read<cs_t>() });
  vp.guest_ds(segment_t{ gdtr, read<ds_t>() });
  vp.guest_es(segment_t{ gdtr, read<es_t>() });
  vp.guest_fs(segment_t{ gdtr, read<fs_t>() });
  vp.guest_gs(segment_t{ gdtr, read<gs_t>() });
  vp.guest_ss(segment_t{ gdtr, read<ss_t>() });
  vp.guest_tr(segment_t{ gdtr, read<tr_t>() });
  vp.guest_ldtr(segment_t{ gdtr, read<ldtr_t>() });

  return {};
}

void vmexit_passthrough_handler::teardown(vcpu_t& vp) noexcept
{
  (void)(vp);
}

void vmexit_passthrough_handler::terminate(vcpu_t& vp) noexcept
{
  (void)(vp);

  vmx::vmcall(vmcall_terminate_id);
}

void vmexit_passthrough_handler::handle_exception_or_nmi(vcpu_t& vp) noexcept
{
  handle_interrupt(vp);
}

void vmexit_passthrough_handler::handle_external_interrupt(vcpu_t& vp) noexcept
{
  handle_interrupt(vp);
}

void vmexit_passthrough_handler::handle_triple_fault(vcpu_t& vp) noexcept
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

void vmexit_passthrough_handler::handle_interrupt_window(vcpu_t& vp) noexcept
{
  //
  // Make sure there is an interrupt pending.
  //
  hvpp_assert(vp.interrupt_is_pending(vcpu_t::interrupt_queue_external));

  //
  // Guest is in the interruptible state.
  // Dequeue one pending interrupt from the queue
  // and inject it.
  //
  vp.interrupt_inject_pending(vcpu_t::interrupt_queue_external);

  //
  // If queue is empty, disable Interrupt-window exiting.
  //
  if (!vp.interrupt_is_pending(vcpu_t::interrupt_queue_external))
  {
    auto procbased_ctls = vp.processor_based_controls();
    procbased_ctls.interrupt_window_exiting = false;
    vp.processor_based_controls(procbased_ctls);
  }

  vp.suppress_rip_adjust();
}

void vmexit_passthrough_handler::handle_nmi_window(vcpu_t& vp) noexcept
{
  //
  // Make sure there is a NMI pending.
  //
  hvpp_assert(vp.interrupt_is_pending(vcpu_t::interrupt_queue_nmi));

  //
  // Guest is in the interruptible state.
  // Dequeue one pending NMI from the queue
  // and inject it.
  //
  vp.interrupt_inject_pending(vcpu_t::interrupt_queue_nmi);

  //
  // If queue is empty, disable NMI-window exiting.
  //
  if (!vp.interrupt_is_pending(vcpu_t::interrupt_queue_nmi))
  {
    auto procbased_ctls = vp.processor_based_controls();
    procbased_ctls.nmi_window_exiting = false;
    vp.processor_based_controls(procbased_ctls);
  }

  vp.suppress_rip_adjust();
}

void vmexit_passthrough_handler::handle_execute_cpuid(vcpu_t& vp) noexcept
{
  uint32_t cpu_info[4];
  ia32_asm_cpuid_ex(cpu_info,
                    vp.context().eax,
                    vp.context().ecx);

  vp.context().rax = cpu_info[0];
  vp.context().rbx = cpu_info[1];
  vp.context().rcx = cpu_info[2];
  vp.context().rdx = cpu_info[3];
}

void vmexit_passthrough_handler::handle_execute_invd(vcpu_t& vp) noexcept
{
  (void)(vp);

  //
  // The INVD instruction invalidates all internal cache entries,
  // then generates a special-function bus cycle that indicates
  // that external caches also should be invalidated.  The INVD
  // instruction should be used with care.  It does not force a
  // write-back of modified cache lines; therefore, data stored
  // in the caches and not written back to system memory will be
  // lost.  Unless there is a specific requirement or benefit to
  // invalidating the caches without writing back the modified
  // lines (such as, during testing or fault recovery where cache
  // coherency with main memory is not a concern), software should
  // use the WBINVD instruction.
  // (ref: Vol3A[11.5.5(Cache Management Instructions)])
  //
  // TL;DR:
  //   INVD can be dangerous, as it doesn't write the cache content
  //   to the RAM.  Because of this, if we would perform this instruction,
  //   the system would most likely crash after some time.  We can
  //   either ignore the instruction or perform WBINVD - which is what
  //   other hypervisors do too.
  //
  //   Note that INVD instruction does not occur in whole NTOSKRNL
  //   and based on articles below, the occurence of this instruction
  //   should be rare.
  //   The use of this instruction is:
  //     - historical (Cache-as-RAM)
  //     - in boot phase/in firmware (shouldn't apply to us, as we're
  //       already booted)
  //     - related to DMA
  //
  // See also:
  //   - https://stackoverflow.com/questions/41775371/what-use-is-the-invd-instruction
  //   - https://forum.osdev.org/viewtopic.php?f=1&t=22942
  //   - https://sites.utexas.edu/jdm4372/2013/05/30/coherence-with-cached-memory-mapped-io/
  //   - https://groups.google.com/forum/#!topic/comp.lang.asm.x86/otIjgt_-sKM
  //

  ia32_asm_wb_invd();
}

void vmexit_passthrough_handler::handle_execute_invlpg(vcpu_t& vp) noexcept
{
  auto linear_address = vp.exit_qualification().linear_address;

  //
  // Invalidate all mappings to the address associated with the VPID
  // of the guest.  Calling INVLPG wouldn't be dangerous here - but
  // it's unnecessary, because:
  //   - that would've been superfluous
  //   - if hypervisor happened to have different address space (i.e.
  //     it wouldn't be "identity-mapped" with current OS) in which it
  //     would legitimately used "linear_address" (for some variable,
  //     stack, ...) we would be causing unnecessary flush of that TLB
  //     entry
  //

  vmx::invvpid_individual_address(vp.vcpu_id(), linear_address);
}

void vmexit_passthrough_handler::handle_execute_rdtsc(vcpu_t& vp) noexcept
{
  uint64_t tsc = ia32_asm_read_tsc();
  vp.context().rax = tsc & 0xffffffff;
  vp.context().rdx = tsc >> 32;
}

void vmexit_passthrough_handler::handle_execute_vmcall(vcpu_t& vp) noexcept
{
  if (vp.context().rcx == vmcall_terminate_id &&
      vp.guest_cs().selector.request_privilege_level == 0)
  {
    vp.vmx_leave();
  }
  else if (vp.context().rcx == vmcall_breakpoint_id)
  {
    debugger::breakpoint();
  }
  else
  {
    handle_vm_fallback(vp);
  }
}

void vmexit_passthrough_handler::handle_mov_cr(vcpu_t& vp) noexcept
{
  auto  exit_qualification = vp.exit_qualification().mov_cr;
  auto& gp_register = vp.context().gp_register[exit_qualification.gp_register];

  switch (exit_qualification.access_type)
  {
    case vmx::exit_qualification_mov_cr_t::access_to_cr:
    {
      switch (exit_qualification.cr_number)
      {
        case 0:
        {
          vp.guest_cr0({ gp_register });
          vp.cr0_shadow({ gp_register });

          break;
        }

        case 3:
        {
          //
          // If CR4.PCIDE = 1, bit 63 of the source operand to MOV
          // to CR3 determines whether the instruction invalidates
          // entries in the TLBs and the paging-structure caches.
          // The instruction does not modify bit 63 of CR3, which
          // is reserved and always 0.
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

          //
          // Some instructions invalidate all entries in the TLBs
          // and paging-structure caches-except for global translations.
          // An example is the MOV to CR3 instruction.
          // Emulation of such an instruction may require execution of
          // the INVVPID instruction as follows:
          //   - The INVVPID type is single-context-retaining-globals (3).
          //   - The VPID in the INVVPID descriptor is the one assigned to
          //     the virtual processor whose execution is being emulated.
          // (ref: Vol3C[28.3.3.3(Guidelines for Use of the INVVPID Instruction)])
          //
          vmx::invvpid_single_context_retaining_globals(vp.vcpu_id());

          break;
        }

        case 4:
        {
          //
          // Some instructions invalidate all entries in the TLBs and
          // paging-structure caches-including for global translations.
          // An example is the MOV to CR4 instruction if the value of
          // value of bit 4 (page global enable-PGE) is changing.
          // Emulation of such an instruction may require execution of
          // the INVVPID instruction as follows:
          //   - The INVVPID type is single-context (1).
          //   - The VPID in the INVVPID descriptor is the one assigned to
          //     the virtual processor whose execution is being emulated.
          // (ref: Vol3C[28.3.3.3(Guidelines for Use of the INVVPID Instruction)])
          //
          auto new_cr4 = cr4_t{ gp_register };
          bool pge_changed = new_cr4.page_global_enable != vp.guest_cr4().page_global_enable;

          if (pge_changed)
          {
            vmx::invvpid_single_context(vp.vcpu_id());
          }

          vp.guest_cr4(new_cr4);
          vp.cr4_shadow(new_cr4);

          break;
        }

        case 8:
        {
          /* unimplemented */
          break;
        }
      }

      break;
    }

    case vmx::exit_qualification_mov_cr_t::access_from_cr:
    {
      switch (exit_qualification.cr_number)
      {
        case 3: gp_register = vp.guest_cr3().flags; break;
        case 8: /* unimplemented */ break;
      }

      break;
    }

    case vmx::exit_qualification_mov_cr_t::access_clts:
    {
      auto cr0 = vp.guest_cr0();
      cr0.task_switched = false;
      vp.guest_cr0(cr0);
      vp.cr0_shadow(cr0);

      break;
    }

    case vmx::exit_qualification_mov_cr_t::access_lmsw:
    {
      auto msw = static_cast<uint16_t>(exit_qualification.lmsw_source_data);
      auto cr0 = vp.guest_cr0();

      //
      // Loads the source operand into the machine status word,
      // bits 0 through 15 of register CR0. The source operand
      // can be a 16-bit general-purpose register or a memory
      // location.  Only the low-order 4 bits of the source
      // operand (which contains the PE, MP, EM, and TS flags)
      // are loaded into CR0.  The PG, CD, NW, AM, WP, NE, and
      // ET flags of CR0 are not affected.  The operand-size
      // attribute has no effect on this instruction.  If the
      // PE flag of the source operand (bit 0) is set to 1, the
      // instruction causes the processor to switch to protected
      // mode.  While in protected mode, the LMSW instruction
      // cannot be used to clear the PE flag and force a switch
      // back to real-address mode.
      // (ref: Vol2A[(LMSW-Load Machine Status Word)])
      //
      // TL;DR:
      //   CR0[0:3] <- SRC[0:3];
      //
      //   ...except if CR0.PE (bit 0) is already 1 - then do not
      //   change that bit (lmsw can't be used to switch back to
      //   real mode from the protected mode.
      //

      cr0.flags &=      ~0b1110;
      cr0.flags |= msw & 0b1111;

      vp.guest_cr0(cr0);
      vp.cr0_shadow(cr0);

      break;
    }
  }
}

void vmexit_passthrough_handler::handle_mov_dr(vcpu_t& vp) noexcept
{
  auto  exit_qualification = vp.exit_qualification().mov_dr;
  auto& gp_register = vp.context().gp_register[exit_qualification.gp_register];

  //
  // The MOV DR instruction causes a VM exit if the "MOV-DR exiting"
  // VM-execution control is 1.  Such VM exits represent an exception
  // to the principles identified in Section 25.1.1 (Relative Priority
  // of Faults and VM Exits) in that they take priority over the
  // following: general-protection exceptions based on privilege level;
  // and invalid-opcode exceptions that occur because CR4.DE = 1 and the
  // instruction specified access to DR4 or DR5.
  // (ref: Vol3C[25.1.3(Instructions That Cause VM Exits Conditionally)])
  //
  // TL;DR:
  //   CPU usually prioritizes exceptions.  For example RDMSR executed
  //   at CPL = 3 won't cause VM-exit - it causes #GP instead.  MOV DR
  //   is exception to this rule, as it ALWAYS cause VM-exit.
  //
  //   Normally, CPU allows you to write to DR registers only at CPL=0,
  //   otherwise it causes #GP.  Therefore we'll simulate the exact same
  //   behavior here.
  //

  if (vp.guest_cs().access.descriptor_privilege_level != 0)
  {
    vp.interrupt_inject(interrupt::general_protection);
    vp.suppress_rip_adjust();
    return;
  }

  //
  // Debug registers DR4 and DR5 are reserved when debug extensions
  // are enabled (when the DE flag in control register CR4 is set)
  // and attempts to reference the DR4 and DR5 registers cause
  // invalid-opcode exceptions (#UD).
  // When debug extensions are not enabled (when the DE flag is clear),
  // these registers are aliased to debug registers DR6 and DR7.
  // (ref: Vol3B[17.2.2(Debug Registers DR4 and DR5)])
  //

  if (exit_qualification.dr_number == 4 ||
      exit_qualification.dr_number == 5)
  {
    if (vp.guest_cr4().debugging_extensions)
    {
      vp.interrupt_inject(interrupt::invalid_opcode);
      vp.suppress_rip_adjust();
      return;
    }
    else
    {
      exit_qualification.dr_number += 2;
    }
  }

  //
  // Enables (when set) debug-register protection, which causes a
  // debug exception to be generated prior to any MOV instruction
  // that accesses a debug register.  When such a condition is
  // detected, the BD flag in debug status register DR6 is set prior
  // to generating the exception.  This condition is provided to
  // support in-circuit emulators.
  // When the emulator needs to access the debug registers, emulator
  // software can set the GD flag to prevent interference from the
  // program currently executing on the processor.
  // The processor clears the GD flag upon entering to the debug
  // exception handler, to allow the handler access to the debug
  // registers.
  // (ref: Vol3B[17.2.4(Debug Control Register (DR7)])
  //

  if (vp.guest_dr7().general_detect)
  {
    auto dr6 = read<dr6_t>();
    dr6.breakpoint_condition = 0;
    dr6.debug_register_access_detected = true;
    write<dr6_t>(dr6);

    auto dr7 = vp.guest_dr7();
    dr7.general_detect = false;
    vp.guest_dr7(dr7);

    vp.interrupt_inject(interrupt::debug);
    vp.suppress_rip_adjust();
    return;
  }

  //
  // In 64-bit mode, the upper 32 bits of DR6 and DR7 are reserved
  // and must be written with zeros.  Writing 1 to any of the upper
  // 32 bits results in a #GP(0) exception.
  // (ref: Vol3B[17.2.6(Debug Registers and Intel® 64 Processors)])
  //
  if (exit_qualification.access_type == vmx::exit_qualification_mov_dr_t::access_to_dr && (
      exit_qualification.dr_number == 6 ||
      exit_qualification.dr_number == 7) &&
      (gp_register >> 32) != 0)
  {
    vp.interrupt_inject(interrupt::general_protection);
    vp.suppress_rip_adjust();
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
        case 6: write<dr6_t>(vmx::adjust(dr6_t{ gp_register })); break;
        case 7: vp.guest_dr7(vmx::adjust(dr7_t{ gp_register })); break;
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

void vmexit_passthrough_handler::handle_execute_io_instruction(vcpu_t& vp) noexcept
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
  // We don't check if CPL == 0 here, because the CPU would
  // raise #GP instead of VM-exit.
  //
  // See Vol3C[25.1.1(Relative Priority of Faults and VM Exits)]
  //

  //
  // Resolve address of the source or destination.
  //
  if (exit_qualification.string_instruction)
  {
    //
    // String operations always operate either on RDI (in) or
    // RSI (out) registers.
    //
    port_value.as_ptr =
      exit_qualification.access_type == vmx::exit_qualification_io_instruction_t::access_in
        ? vp.context().rdi_as_pointer
        : vp.context().rsi_as_pointer;
  }
  else
  {
    //
    // Save pointer to the RAX register.
    //
    port_value.as_ptr = &vp.context().rax;
  }

  //
  // Resolve port as a nice 16bit number.
  //
  auto port = static_cast<uint16_t>(exit_qualification.port_number);

  //
  // Resolve number of bytes to send/receive.
  // REP prefixed instructions always take their count
  // from *CX register.
  //
  auto count = exit_qualification.rep_prefixed
    ? vp.context().ecx
    : 1;

  auto size = static_cast<uint32_t>(exit_qualification.size_of_access) + 1;

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND

  (void)(port_value);
  (void)(port);
  (void)(count);
  (void)(size);

  ia32_asm_io_with_context(exit_qualification, vp.context());

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
        // Note that port_value holds pointer to the
        // vp.context().rax member, therefore we're
        // directly overwriting the RAX value.
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
        // Note that port_value holds pointer to the
        // vp.context().rax member, therefore we're
        // directly reading from the RAX value.
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
    // If the DF (direction flag) is set, decrement,
    // otherwise increment.
    //
    // For in the register is RDI, for out it's RSI.
    //
    auto& gp_register =
      exit_qualification.access_type == vmx::exit_qualification_io_instruction_t::access_in
        ? vp.context().rdi
        : vp.context().rsi;

    if (vp.context().rflags.direction_flag)
    {
      gp_register -= count * size;
    }
    else
    {
      gp_register += count * size;
    }

    //
    // We've sent/received everything, reset counter register
    // to 0.
    //
    if (exit_qualification.rep_prefixed)
    {
      vp.context().rcx = 0;
    }
  }

#endif
}

void vmexit_passthrough_handler::handle_execute_rdmsr(vcpu_t& vp) noexcept
{
  uint32_t msr_id = vp.context().ecx;
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

  vp.context().rax = msr_value & 0xffffffff;
  vp.context().rdx = msr_value >> 32;
}

void vmexit_passthrough_handler::handle_execute_wrmsr(vcpu_t& vp) noexcept
{
  uint32_t msr_id    = vp.context().ecx;
  uint64_t msr_value =
    vp.context().rax |
    vp.context().rdx << 32;

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

void vmexit_passthrough_handler::handle_gdtr_idtr_access(vcpu_t& vp) noexcept
{
  auto instruction_info = vp.exit_instruction_info().gdtr_idtr_access;
  auto guest_va = vp.exit_instruction_info_guest_va();

  union
  {
    gdtr32_t gdtr32;
    idtr32_t idtr32;

    gdtr64_t gdtr64;
    idtr64_t idtr64;
  };

  cr3_guard _{ vp.guest_cr3() };

  //
  // In legacy or compatibility mode, the destination operand
  // is a 6-byte memory location. If the operand-size attribute
  // is 16 or 32 bits, the 16-bit limit field of the register
  // is stored in the low 2 bytes of the memory location and the
  // 32-bit base address is stored in the high 4 bytes.
  //
  // In 64-bit mode, the operand size is fixed at 8+2 bytes.
  // The instruction stores an 8-byte base and a 2-byte limit.
  // (ref: Vol2B[(SGDT-Store Global Descriptor Table Register)])
  //
  // See also: Vol2B[(SIDT-Store Interrupt Descriptor Table Register)]
  //
  // TL;DR:
  //   SGDT and SIDT instructions can be executed both from
  //   x64 (long) mode and x86 mode (i.e.: by WoW64 processes).
  //   But descriptor table register on x86 has different size
  //   (6 bytes) than on x64 (10 bytes).
  //   The size of written bytes must be correctly emulated.
  //
  const auto guest_in_long_mode = [&vp]() noexcept -> bool {
    const auto   selector = vp.guest_segment_selector(context_t::seg_cs);
    const auto&  descriptor_entry = vp.guest_gdtr()[selector];

    return descriptor_entry.access.long_mode;
  };

  switch (instruction_info.instruction)
  {
    case vmx::instruction_info_gdtr_idtr_access_t::instruction_sgdt:
      gdtr64 = vp.guest_gdtr();
      memcpy(guest_va, &gdtr64, guest_in_long_mode() ? sizeof(gdtr64)
                                                     : sizeof(gdtr32));
      break;

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_sidt:
      idtr64 = vp.guest_idtr();
      memcpy(guest_va, &idtr64, guest_in_long_mode() ? sizeof(idtr64)
                                                     : sizeof(idtr32));
      break;

    //
    // LGDT and LIDT instructions can be performed only when CPL=0
    // (i.e.: in kernel mode).  Kernel mode code in Windows always
    // runs in long mode, therefore we don't emulate the legacy or
    // compatibility mode here.
    //

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_lgdt:
      memcpy(&gdtr64, guest_va, sizeof(gdtr64));
      vp.guest_gdtr(gdtr64);
      break;

    case vmx::instruction_info_gdtr_idtr_access_t::instruction_lidt:
      memcpy(&idtr64, guest_va, sizeof(idtr64));
      vp.guest_idtr(idtr64);
      break;
  }
}

void vmexit_passthrough_handler::handle_ldtr_tr_access(vcpu_t& vp) noexcept
{
  auto instruction_info = vp.exit_instruction_info().ldtr_tr_access;

  cr3_guard _{ vp.guest_cr3() };

  auto& low_word =
    instruction_info.access_type == vmx::instruction_info_t::access_memory
      ? *reinterpret_cast<uint16_t*>(vp.exit_instruction_info_guest_va())
      // instruction_info.access_type == vmx::instruction_info::access_register
      :  reinterpret_cast<uint16_t&>(vp.context().gp_register[instruction_info.register_1]);

  switch (instruction_info.instruction)
  {
    case vmx::instruction_info_ldtr_tr_access_t::instruction_sldt:
      low_word = vp.guest_segment_selector(context_t::seg_ldtr).flags;
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_str:
      low_word = vp.guest_segment_selector(context_t::seg_tr).flags;
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_lldt:
      vp.guest_segment_selector(context_t::seg_ldtr, segment_selector_t{ low_word });
      break;

    case vmx::instruction_info_ldtr_tr_access_t::instruction_ltr:
      {
        //
        // After the segment selector is loaded in the task register,
        // the processor uses the segment selector to locate the segment
        // descriptor for the TSS in the global descriptor table(GDT).
        // It then loads the segment limit and base address for the TSS
        // from the segment descriptor into the task register.  The task
        // pointed to by the task register is marked busy, but a switch
        // to the task does not occur.
        // (ref: Vol2A[(LTR-Load Task Register)])
        //
        // TL;DR:
        //   LTR instruction sets busy bit in the TSS and we need to
        //   emulate this behavior.
        //
        auto selector = segment_selector_t{ low_word };
        vp.guest_segment_selector(context_t::seg_tr, selector);

        auto& descriptor = vp.guest_gdtr()[selector];
        descriptor.access.type |= segment_access_t::type_tss_busy_flag;
      }
      break;
  }
}

void vmexit_passthrough_handler::handle_ept_violation(vcpu_t& vp) noexcept
{
  handle_fallback(vp);
}

void vmexit_passthrough_handler::handle_execute_rdtscp(vcpu_t& vp) noexcept
{
  uint32_t tsc_aux;
  uint64_t tsc = ia32_asm_read_tscp(&tsc_aux);

  vp.context().rax = tsc & 0xffffffff;
  vp.context().rdx = tsc >> 32;
  vp.context().rcx = tsc_aux;
}

void vmexit_passthrough_handler::handle_execute_wbinvd(vcpu_t& vp) noexcept
{
  (void)(vp);

  ia32_asm_wb_invd();
}

void vmexit_passthrough_handler::handle_execute_xsetbv(vcpu_t& vp) noexcept
{
  ia32_asm_write_xcr(vp.context().ecx,
                     vp.context().rdx << 32 |
                     vp.context().rax);
}

void vmexit_passthrough_handler::handle_execute_invpcid(vcpu_t& vp) noexcept
{
  auto instruction_info = vp.exit_instruction_info().invalidate;
  auto guest_va = vp.exit_instruction_info_guest_va();

  invpcid_t type = static_cast<invpcid_t>(vp.context().gp_register[instruction_info.register_2]);
  invpcid_desc_t descriptor;

  //
  // Error checking according to:
  // Vol2A[(INVPCID-Invalidate Process-Context Identifier)]
  // "64-Bit Mode Exceptions"
  //

  //
  // #GP(0) ... If an invalid type is specified in the register
  // operand, i.e., INVPCID_TYPE > 3.
  //
  if (type > invpcid_t::all_contexts_retaining_globals)
  {
    goto inject_general_protection;
  }

  //
  // TODO: fetch the descriptor safely, inject #PF on error.
  //
  memcpy(&descriptor, guest_va, sizeof(descriptor));

  //
  // #GP(0) ... If bits 63:12 of INVPCID_DESC are not all zero.
  //
  if (descriptor.reserved != 0)
  {
    goto inject_general_protection;
  }

  //
  // #GP(0) ... If CR4.PCIDE=0, INVPCID_TYPE is either 0 or 1,
  // and INVPCID_DESC[11:0] is not zero.
  //
  if ((type == invpcid_t::individual_address ||
       type == invpcid_t::single_context) &&
       descriptor.pcid != 0 &&
       !vp.guest_cr4().pcid_enable)
  {
    goto inject_general_protection;
  }

  //
  // Emulate INVPCID instruction using IVVPID instruction.
  //
  // Note that when type == invpcid_t::single_context, the
  // INVVPID we call will unfortunatelly invalidate TLB entries
  // for all PCIDs.  That's because there isn't such instruction
  // which would invalidate TLB entries based on [ PCID, VPID ]
  // pair.
  //
  // We would be probably fine if we just straight executed the
  // INVPCID instruction here, as we're virtualizing just single
  // OS.  That way we would be risking invalidation of ours
  // (hypervisors) cached TLB entries, but that shouldn't cause
  // any serious problems.
  //

  switch (type)
  {
    case invpcid_t::individual_address:
      //
      // TODO:
      // #GP(0) ... If INVPCID_TYPE is 0 and the linear address
      // in INVPCID_DESC[127:64] is not canonical.
      //
      vmx::invvpid_individual_address(vp.vcpu_id(), descriptor.linear_address);
      break;

    case invpcid_t::single_context:
      vmx::invvpid_single_context(vp.vcpu_id());
      break;

    case invpcid_t::all_contexts:
      vmx::invvpid_single_context(vp.vcpu_id());
      break;

    case invpcid_t::all_contexts_retaining_globals:
      vmx::invvpid_single_context_retaining_globals(vp.vcpu_id());
      break;
  }

  return;

inject_general_protection:
  vp.interrupt_inject(interrupt::general_protection);
  vp.suppress_rip_adjust();
}

void vmexit_passthrough_handler::handle_execute_vmclear(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmlaunch(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmptrld(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmptrst(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmread(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmresume(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmwrite(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmxoff(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmxon(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_invept(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_invvpid(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_execute_vmfunc(vcpu_t& vp) noexcept
{ handle_vm_fallback(vp); }

void vmexit_passthrough_handler::handle_vm_fallback(vcpu_t& vp) noexcept
{
  vp.interrupt_inject(interrupt::invalid_opcode);
  vp.suppress_rip_adjust();
}

void vmexit_passthrough_handler::handle_interrupt(vcpu_t& vp) noexcept
{
  //
  // Common code for handling all exceptions and interrupts.
  //

  const auto interrupt = vp.interrupt_info();

  switch (interrupt.type())
  {
    case vmx::interrupt_type::nmi:
      switch (interrupt.vector())
      {
        case exception_vector::nmi_interrupt:
          break;

        default:
          hvpp_assert(0);
          break;
      }

    case vmx::interrupt_type::hardware_exception:
      switch (interrupt.vector())
      {
        case exception_vector::invalid_opcode:
        {
          //
          // Have to fetch the full instruction to skip the prefixes.
          //
          uint8_t buffer[15];

          auto instruction_length = static_cast<size_t>(vp.exit_instruction_length());
          auto read_size = std::min(instruction_length, sizeof(buffer));

          if (auto err_va = vp.guest_read_memory(vp.context().rip, buffer, read_size))
          {
            hvpp_trace("handle_interrupt (invalid_opcode) - read_guest_memory(%p, %u) failed, injecting #PF",
                       vp.context().rip,
                       instruction_length);

            write<cr2_t>({ err_va.value() });
            vp.interrupt_inject(interrupt::page_fault);
            vp.suppress_rip_adjust();

            //
            // `vp.guest_resume()' can be also used instead of `return'.
            //

            return;
          }

          //
          // Compare copied memory to `syscall' & `sysret' instructions.
          //
          if (detail::is_syscall_instruction(&buffer[0], &buffer[read_size]))
          {
            handle_emulate_syscall(vp);
            vp.suppress_rip_adjust();
            return;
          }
          else if (detail::is_sysret_instruction(&buffer[0], &buffer[read_size]))
          {
            handle_emulate_sysret(vp);
            vp.suppress_rip_adjust();
            return;
          }

          break;
        }

        case exception_vector::general_protection:
        {
          //
          // Have to fetch the full instruction to skip the prefixes.
          //
          uint8_t buffer[15];

          auto instruction_length = static_cast<size_t>(vp.exit_instruction_length());
          auto read_size = std::min(instruction_length, sizeof(buffer));

          if (auto err_va = vp.guest_read_memory(vp.context().rip, buffer, read_size))
          {
            hvpp_trace("handle_interrupt (general_protection) - read_guest_memory(%p, %u) failed, injecting #PF",
                       vp.context().rip,
                       instruction_length);

            write<cr2_t>({ err_va.value() });
            vp.interrupt_inject(interrupt::page_fault);
            vp.suppress_rip_adjust();

            //
            // `vp.guest_resume()' can be also used instead of `return'.
            //

            return;
          }

          //
          // Compare copied memory to `rdtsc' & `rdtscp' instructions.
          //
          if (detail::is_rdtsc_instruction(&buffer[0], &buffer[read_size]))
          {
            handle_emulate_rdtsc(vp);
            return;
          }
          else if (detail::is_rdtscp_instruction(&buffer[0], &buffer[read_size]))
          {
            handle_emulate_rdtscp(vp);
            return;
          }

#ifdef HVPP_ENABLE_VMWARE_WORKAROUND

//           //
//           // VMWare I/O backdoor (port 0x5658/0x5659) workaround.
//           //
//           cr3_guard _{ vp.guest_cr3() };
//
//           vmx::exit_qualification_io_instruction_t exit_qualification;
//           if (try_decode_io_instruction(vp.context(), exit_qualification))
//           {
//             ia32_asm_io_with_context(exit_qualification, vp.context());
//             return;
//           }

#endif

          break;
        }

        case exception_vector::page_fault:
        {
          write<cr2_t>({ vp.exit_qualification().linear_address });
          break;
        }

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
  vp.interrupt_inject(interrupt);

  //
  // Do not increment rip by exit_instruction_length() in
  // vcpu::entry_host().
  //
  // The RIP is controlled by entry_instruction_length()
  // instead, which is taken from interrupt_info::rip_adjust().
  //
  vp.suppress_rip_adjust();
}

//
// SYSCALL/SYSRET emulation inspired by:
//   https://revers.engineering/syscall-hooking-via-extended-feature-enable-register-efer/
//

void vmexit_passthrough_handler::handle_emulate_syscall(vcpu_t& vp) noexcept
{
  //
  // Save the address of the instruction following SYSCALL
  // into RCX and then load RIP from MSR_LSTAR.
  //
  const auto lstar = msr::read<msr::lstar_t>();
  vp.context().rcx = vp.context().rip + vp.exit_instruction_length();
  vp.context().rip = lstar;

  //
  // Save RFLAGS into R11 and then mask RFLAGS using MSR_FMASK.
  //
  const auto fmask = msr::read<msr::fmask_t>();
  vp.context().r11 = vp.context().rflags.flags;
  vp.context().rflags.flags &= ~fmask.flags;

  //
  // Load the CS and SS selectors with values derived from
  // bits 47:32 of MSR_STAR.
  //
  const auto star = msr::read<msr::star_t>();

  //
  // Verbose version of:
  //   cs.access = segment_access_vmx_t{ 0xa09b };
  //
  // cs.access.type                       = segment_access_vmx_t::type_execute_read_accessed;
  // cs.access.descriptor_type            = segment_access_vmx_t::descriptor_type_code_or_data;
  // cs.access.descriptor_privilege_level = 0;                  // DPL0 == kernel mode
  // cs.access.present                    = 1;
  // // cs.access.limit_high              = 0;                  // Unchanged
  // // cs.access.available_bit           = 0;                  // Unchanged
  // cs.access.long_mode                  = 1;
  // cs.access.default_big                = 0;
  // cs.access.granularity                = segment_access_vmx_t::granularity_4kb;
  //

  auto cs = vp.guest_cs();
  cs.base_address = nullptr;                                    // Flat segment
  cs.limit        = uint32_t(0xffffffff);                       // 4GB limit
  cs.access       = segment_access_vmx_t{ 0xa09b };             // L+DB+P+S+DPL0+Code
  cs.selector     = cs_t{ uint16_t((star >> 32) & ~3) };        // STAR[47:32] & ~RPL3
  vp.guest_cs(cs);

  //
  // Verbose version of:
  //   ss.access = segment_access_vmx_t{ 0xc093 };
  //
  // ss.access.type                       = segment_access_vmx_t::type_read_write_accessed;
  // ss.access.descriptor_type            = segment_access_vmx_t::descriptor_type_code_or_data;
  // ss.access.descriptor_privilege_level = 0;                  // DPL0 == kernel mode
  // ss.access.present                    = 1;
  // // ss.access.limit_high              = 0;                  // Unchanged
  // // ss.access.available_bit           = 0;                  // Unchanged
  // // ss.access.long_mode               = 0;                  // Unchanged
  // ss.access.default_big                = 1;
  // ss.access.granularity                = segment_access_vmx_t::granularity_4kb;
  //

  auto ss = vp.guest_ss();
  ss.base_address = nullptr;                                    // Flat segment
  ss.limit        = uint32_t(0xffffffff);                       // 4GB limit
  ss.access       = segment_access_vmx_t{ 0xc093 };             // G+DB+P+S+DPL0+Data
  ss.selector     = ss_t{ uint16_t(((star >> 32) & ~3) + 8) };  // STAR[47:32] + 8
  vp.guest_ss(ss);
}

void vmexit_passthrough_handler::handle_emulate_sysret(vcpu_t& vp) noexcept
{
  //
  // Load RIP from RCX.
  //
  vp.context().rip = vp.context().rcx;

  //
  // Load RFLAGS from R11. Clear RF, VM, reserved bits.
  //
  vp.context().rflags.flags = vp.context().r11;
  vp.context().rflags.flags &= ~rflags_t::reserved_bits;
  vp.context().rflags.flags |=  rflags_t::fixed_bits;

  //
  // SYSRET loads the CS and SS selectors with values
  // derived from bits 63:48 of MSR_STAR.
  //
  const auto star = msr::read<msr::star_t>();

  //
  // Verbose version of:
  //   cs.access = segment_access_vmx_t{ 0xa0fb };
  //
  // cs.access.type                       = segment_access_vmx_t::type_execute_read_accessed;
  // cs.access.descriptor_type            = segment_access_vmx_t::descriptor_type_code_or_data;
  // cs.access.descriptor_privilege_level = 3;                  // DPL3 == user mode
  // cs.access.present                    = 1;
  // // cs.access.limit_high              = 0;                  // Unchanged
  // // cs.access.available_bit           = 0;                  // Unchanged
  // cs.access.long_mode                  = 1;                  // Enforce return to long (64-bit) mode
  // cs.access.default_big                = 0;
  // cs.access.granularity                = segment_access_vmx_t::granularity_4kb;
  //

  auto cs = vp.guest_cs();
  cs.base_address = nullptr;                                    // Flat segment
  cs.limit        = uint32_t(0xffffffff);                       // 4GB limit
  cs.access       = segment_access_vmx_t{ 0xa0fb };             // L+DB+P+S+DPL3+Code
  cs.selector     = cs_t{ uint16_t(((star >> 48) + 16) | 3) };  // (STAR[63:48]+16) | 3 (* RPL forced to 3 *)
  vp.guest_cs(cs);

  //
  // Verbose version of:
  //   ss.access = segment_access_vmx_t{ 0xc0f3 };
  //
  // ss.access.type                       = segment_access_vmx_t::type_read_write_accessed;
  // ss.access.descriptor_type            = segment_access_vmx_t::descriptor_type_code_or_data;
  // ss.access.descriptor_privilege_level = 3;                  // DPL3 == user mode
  // ss.access.present                    = 1;
  // // ss.access.limit_high              = 0;                  // Unchanged
  // // ss.access.available_bit           = 0;                  // Unchanged
  // // ss.access.long_mode               = 0;                  // Unchanged
  // ss.access.default_big                = 1;
  // ss.access.granularity                = segment_access_vmx_t::granularity_4kb;
  //

  auto ss = vp.guest_ss();
  ss.base_address = nullptr;                                    // Flat segment
  ss.limit        = uint32_t(0xffffffff);                       // 4GB limit
  ss.access       = segment_access_vmx_t{ 0xc0f3 };             // G+DB+P+S+DPL3+Data
  ss.selector     = ss_t{ uint16_t(((star >> 48) + 8) | 3) };   // (STAR[63:48]+8) | 3 (* RPL forced to 3 *)
  vp.guest_ss(ss);
}

void vmexit_passthrough_handler::handle_emulate_rdtsc(vcpu_t& vp) noexcept
{
  handle_execute_rdtsc(vp);
}

void vmexit_passthrough_handler::handle_emulate_rdtscp(vcpu_t& vp) noexcept
{
  handle_execute_rdtscp(vp);
}

}
