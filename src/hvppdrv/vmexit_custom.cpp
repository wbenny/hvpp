#include "vmexit_custom.h"

#include <hvpp/lib/cr3_guard.h>
#include <hvpp/lib/mp.h>
#include <hvpp/lib/log.h>

auto vmexit_custom_handler::setup(vcpu_t& vp) noexcept -> error_code_t
{
  base_type::setup(vp);

  //
  // Set per-VCPU data and mirror current physical memory in EPT.
  //
  auto data = new per_vcpu_data{};
  data->ept.map_identity();
  data->page_exec = 0;
  data->page_read = 0;
  vp.user_data(data);

  //
  // Enable EPT.
  //
  vp.ept(data->ept);
  vp.ept_enable();

#if 1
  //
  // Enable exitting on 0x64 I/O port (keyboard).
  //
  auto procbased_ctls = vp.processor_based_controls();
  procbased_ctls.use_io_bitmaps = true;
  vp.processor_based_controls(procbased_ctls);

  vmx::io_bitmap_t io_bitmap{};
  bitmap<>(io_bitmap.a).set(0x64);

  vp.io_bitmap(io_bitmap);
#else
  //
  // Turn on VM-exit on everything we support.
  //

  auto procbased_ctls = vp.processor_based_controls();

  //
  // Since VMWare handles rdtsc(p) instructions by its own
  // magical way, we'll disable our own handling.  Setting
  // this in VMWare makes the guest OS completely bananas.
  //
  // procbased_ctls.rdtsc_exiting = true;

  //
  // Use either "use_io_bitmaps" or "unconditional_io_exiting",
  // try to avoid using both of them.
  //

// #define USE_IO_BITMAPS
// #define DISABLE_GP_EXITING

#ifdef USE_IO_BITMAPS
  procbased_ctls.use_io_bitmaps = true;
#else
  procbased_ctls.unconditional_io_exiting = true;
#endif
  procbased_ctls.mov_dr_exiting = true;
  procbased_ctls.cr3_load_exiting = true;
  procbased_ctls.cr3_store_exiting = true;
  procbased_ctls.invlpg_exiting = true;
  vp.processor_based_controls(procbased_ctls);

  auto procbased_ctls2 = vp.processor_based_controls2();
  procbased_ctls2.descriptor_table_exiting = true;
  vp.processor_based_controls2(procbased_ctls2);

  vmx::msr_bitmap_t msr_bitmap{};
  memset(msr_bitmap.data, 0xff, sizeof(msr_bitmap));
  vp.msr_bitmap(msr_bitmap);

#ifdef USE_IO_BITMAPS
  vmx::io_bitmap_t io_bitmap{};
  memset(io_bitmap.data, 0xff, sizeof(io_bitmap));

  //
  // Disable VMWare backdoor.
  //
  bitmap<>(io_bitmap.a).clear(0x5658);
  bitmap<>(io_bitmap.a).clear(0x5659);

  vp.io_bitmap(io_bitmap);
#endif

#ifdef DISABLE_GP_EXITING
  //
  // Catch all exceptions except #GP.
  //
  vmx::exception_bitmap_t exception_bitmap{ ~0ul };
  exception_bitmap.general_protection = false;
  vp.exception_bitmap(exception_bitmap);
#else
  //
  // Catch all exceptions.
  //
  vp.exception_bitmap(vmx::exception_bitmap_t{ ~0ul });
#endif

  //
  // VM-execution control fields include guest/host masks
  // and read shadows for the CR0 and CR4 registers.
  // These fields control executions of instructions that
  // access those registers (including CLTS, LMSW, MOV CR,
  // and SMSW).
  // They are 64 bits on processors that support Intel 64
  // architecture and 32 bits on processors that do not.
  //
  // In general, bits set to 1 in a guest/host mask correspond
  // to bits "owned" by the host:
  //   - Guest attempts to set them (using CLTS, LMSW, or
  //     MOV to CR) to values differing from the corresponding
  //     bits in the corresponding read shadow cause VM exits.
  //   - Guest reads (using MOV from CR or SMSW) return values
  //     for these bits from the corresponding read shadow.
  //
  // Bits cleared to 0 correspond to bits "owned" by the
  // guest; guest attempts to modify them succeed and guest
  // reads return values for these bits from the control
  // register itself.
  // (ref: Vol3C[24.6.6(Guest/Host Masks and Read Shadows for CR0 and CR4)])
  //
  // TL;DR:
  //   When bit in guest/host mask is set, write to the control
  //   register causes VM-exit.  Mov FROM CR0 and CR4 returns
  //   values in the shadow register values.
  //
  // Note that SHADOW register value and REAL register value may
  // differ.  The guest will behave according to the REAL control
  // register value.  Only read from that register will return the
  // fake (aka "shadow") value.
  //

  vp.cr0_guest_host_mask(cr0_t{ ~0ull });
  vp.cr4_guest_host_mask(cr4_t{ ~0ull });
#endif

  return {};
}

void vmexit_custom_handler::teardown(vcpu_t& vp) noexcept
{
  auto& data = user_data(vp);
  delete &data;

  vp.user_data(nullptr);
}

void vmexit_custom_handler::handle_execute_cpuid(vcpu_t& vp) noexcept
{
  if (vp.context().eax == 'ppvh')
  {
    //
    // "hello from hvpp\0"
    //
    vp.context().rax = 'lleh';
    vp.context().rbx = 'rf o';
    vp.context().rcx = 'h mo';
    vp.context().rdx = 'ppv';
  }
  else
  {
    base_type::handle_execute_cpuid(vp);
  }
}

void vmexit_custom_handler::handle_execute_vmcall(vcpu_t& vp) noexcept
{
  auto& data = user_data(vp);

  switch (vp.context().rcx)
  {
    case 0xc1:
      {
        cr3_guard _{ vp.guest_cr3() };

        data.page_read = pa_t::from_va(vp.context().rdx_as_pointer);
        data.page_exec = pa_t::from_va(vp.context().r8_as_pointer);
      }

      hvpp_trace("vmcall (hook) EXEC: 0x%p READ: 0x%p", data.page_exec.value(), data.page_read.value());

      //
      // Split the 2MB page where the code we want to hook resides.
      //
      vp.ept().split_2mb_to_4kb(data.page_exec & ept_pd_t::mask, data.page_exec & ept_pd_t::mask);

      //
      // Set execute-only access on the page we want to hook.
      //
      vp.ept().map_4kb(data.page_exec, data.page_exec, epte_t::access_type::execute);

      //
      // We've changed EPT structure - mappings derived from EPT need to be
      // invalidated.
      //
      vmx::invept_single_context(vp.ept().ept_pointer());
    break;

    case 0xc2:
      hvpp_trace("vmcall (unhook)");

      //
      // Merge the 4kb pages back to the original 2MB large page.
      // Note that this will also automatically set the access
      // rights to read_write_execute.
      //
      vp.ept().join_4kb_to_2mb(data.page_exec & ept_pd_t::mask, data.page_exec & ept_pd_t::mask);

      //
      // We've changed EPT structure - mappings derived from EPT
      // need to be invalidated.
      //
      vmx::invept_single_context(vp.ept().ept_pointer());
      break;

    default:
      base_type::handle_execute_vmcall(vp);
      return;
  }
}

void vmexit_custom_handler::handle_ept_violation(vcpu_t& vp) noexcept
{
  auto exit_qualification = vp.exit_qualification().ept_violation;
  auto guest_pa = vp.exit_guest_physical_address();
  auto guest_va = vp.exit_guest_linear_address();

  auto& data = user_data(vp);

  if (exit_qualification.data_read || exit_qualification.data_write)
  {
    //
    // Someone requested read or write access to the guest_pa,
    // but the page has execute-only access.  Map the page with
    // the "data.page_read" we've saved before in the VMCALL
    // handler and set the access to RW.
    //
    hvpp_trace("data_read LA: 0x%p PA: 0x%p", guest_va.value(), guest_pa.value());

    vp.ept().map_4kb(data.page_exec, data.page_read, epte_t::access_type::read_write);
  }
  else if (exit_qualification.data_execute)
  {
    //
    // Someone requested execute access to the guest_pa, but
    // the page has only read-write access.  Map the page with
    // the "data.page_execute" we've saved before in the VMCALL
    // handler and set the access to execute-only.
    //
    hvpp_trace("data_execute LA: 0x%p PA: 0x%p", guest_va.value(), guest_pa.value());

    vp.ept().map_4kb(data.page_exec, data.page_exec, epte_t::access_type::execute);
  }

  //
  // An EPT violation invalidates any guest-physical mappings
  // (associated with the current EP4TA) that would be used to
  // translate the guest-physical address that caused the EPT
  // violation.  If that guest-physical address was the translation
  // of a linear address, the EPT violation also invalidates
  // any combined mappings for that linear address associated
  // with the current PCID, the current VPID and the current EP4TA.
  // (ref: Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)])
  //
  //
  // TL;DR:
  //   We don't need to call INVEPT (nor INVVPID) here, because
  //   CPU invalidates mappings for the accessed linear address
  //   for us.
  //
  //   Note1:
  //     In the paragraph above, "EP4TA" is the value of bits
  //     51:12 of EPTP.  These 40 bits contain the address of
  //     the EPT-PML4-table (the notation EP4TA refers to those
  //     40 bits).
  //
  //   Note2:
  //     If we would change any other EPT structure, INVEPT or
  //     INVVPID might be needed.
  //

  //
  // Make the instruction which fetched the memory to be executed
  // again (this time without EPT violation).
  //
  vp.suppress_rip_adjust();
}

auto vmexit_custom_handler::user_data(vcpu_t& vp) noexcept -> per_vcpu_data&
{
  return *reinterpret_cast<per_vcpu_data*>(vp.user_data());
}
