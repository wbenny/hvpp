#include "vcpu.h"
#include "vmexit.h"
#include "ia32/interrupt.h"
#include "lib/assert.h"
#include "lib/bitmap.h"
#include "lib/noopt.h"

#include <iterator> // std::end

namespace hvpp {

//
// Public
//

void vcpu::initialize() noexcept
{
  state_ = vcpu_state::off;
  exit_handler_ = &exit_handler_default_;

  //
  // Assertions.
  //
  [this] ()
  {
    //
    // These methods are never referenced from the C++ code, so the optimizer
    // might think it may remove them completely from the executable file.
    // In fact, they are used in the ASM methods implemented in vcpu.asm file.
    // Following "dummy calls" are an easy way to ensure these methods won't be
    // optimized out.
    //
    do_not_optimize_out(&vcpu::entry_guest);
    do_not_optimize_out(&vcpu::entry_host);

    //
    // Sanity checks for offsets relative to the host/guest stack (see guest_rsp()
    // and host_rsp() methods). These offsets are also hardcoded in the vcpu.asm
    // file. If they are ever changed, the static_assert should be hint to fix
    // them in the vcpu.asm as well.
    //
    constexpr intptr_t VCPU_RSP                         = offsetof(vcpu, stack_) + sizeof(vcpu::stack_);
    constexpr intptr_t VCPU_OFFSET                      = -0x8000;  // -vcpu_stack_size
    constexpr intptr_t VCPU_LAUNCH_CONTEXT_OFFSET       =  0;
    constexpr intptr_t VCPU_EXIT_CONTEXT_OFFSET         =  144;     // sizeof(context);

    static_assert(VCPU_RSP + VCPU_OFFSET                == offsetof(vcpu, stack_));
    static_assert(VCPU_RSP + VCPU_LAUNCH_CONTEXT_OFFSET == offsetof(vcpu, guest_context_));
    static_assert(VCPU_RSP + VCPU_EXIT_CONTEXT_OFFSET   == offsetof(vcpu, exit_context_));
  };
}

void vcpu::destroy() noexcept
{
  //
  // Notify the exit handler that we're about to terminate.
  // Exit handler should invoke VMEXIT in such way, that causes
  // handler to call vcpu::terminate(); e.g. VMCALL with specific
  // index.
  //
  state_ = vcpu_state::terminating;
  exit_handler_->invoke_termination();

  //
  // Deallocate EPT.
  //
  ept_.destroy();
}

void vcpu::launch() noexcept
{
  switch (static_cast<vcpu_state>(guest_context_.capture()))
  {
    case vcpu_state::off:
      setup();
      break;

    case vcpu_state::launching:
      state_ = vcpu_state::running;
      break;

    default:
      hvpp_assert(0);
      break;
  }
}

void vcpu::terminate() noexcept
{
  hvpp_assert(state_ != vcpu_state::off && state_ != vcpu_state::terminated);

  if (state_ != vcpu_state::off && state_ != vcpu_state::terminated)
  {
    //
    // Advance RIP before we exit VMX-root mode.
    //
    exit_context_.rip += exit_instruction_length();

    //
    // When running in VMX root mode, the processor will set limits of the
    // GDT and IDT to 0xffff (notice that there are no Host VMCS fields to
    // set these values). This causes problems with PatchGuard, which will
    // believe that the GDTR and IDTR have been modified by malware, and
    // eventually crash the system. Since we know what the original state
    // of the GDTR and IDTR was, simply restore it now.
    //
    write<gdtr_t>(guest_gdtr());
    write<idtr_t>(guest_idtr());

    //
    // Our callback routine may have interrupted an arbitrary user process,
    // and therefore not a thread running with a systemwide page directory.
    // Therefore if we return back to the original caller after turning off
    // VMX, it will keep our current "host" CR3 value which we set on entry
    // to the PML4 of the SYSTEM process. We want to return back with the
    // correct value of the "guest" CR3, so that the currently executing
    // process continues to run with its expected address space mappings.
    //
    write<cr3_t>(guest_cr3());

    //
    // Turn off VMX-root mode on this logical processor.
    //
    // This instruction brings us back to ring 0 from the "ring -1".
    // Note that in non VMX-root mode, all VMX-instructions (incl.
    // invept/invvpid) raise #UD (invalid instruction opcode exception).
    //
    vmx::off();

    state_ = vcpu_state::terminated;
  }
}

void vcpu::set_exit_handler(vmexit_handler* exit_handler) noexcept
{
  exit_handler_ = exit_handler;
}

//
// Private
//

void vcpu::error() noexcept
{
  state_instruction_error_ = exit_instruction_error();
  hv_log("error: %p\n", state_instruction_error_);
  __debugbreak();
}

void vcpu::setup() noexcept
{
  ept_.initialize();
  ept_.map();

  load_vmcs_host();
  load_vmcs_guest();

  setup_vmcs_host();
  setup_vmcs_guest();

  vmx::invept(vmx::invept_t::all_context);
  vmx::invvpid(vmx::invvpid_t::all_context);

  vmx::vmlaunch();
  error();
}

void vcpu::load_vmcs_host() noexcept
{
  write(vmx::adjust(read<cr0_t>()));
  write(vmx::adjust(read<cr4_t>()));

  auto vmx_basic = msr::read<msr::vmx_basic>();
  vmcs_host_.revision_id = vmx_basic.vmcs_revision_id;

  if (vmx::on(pa_t::from_va(&vmcs_host_)) == vmx::error_code::success)
  {
    state_ = vcpu_state::initializing;
  }
  else
  {
    state_ = vcpu_state::terminated;
    error();
  }
}

void vcpu::load_vmcs_guest() noexcept
{
  auto vmx_basic = msr::read<msr::vmx_basic>();
  vmcs_guest_.revision_id = vmx_basic.vmcs_revision_id;

  hvpp_assert(state_ == vcpu_state::initializing);

  if (vmx::vmclear(pa_t::from_va(&vmcs_guest_)) == vmx::error_code::success &&
      vmx::vmptrld(pa_t::from_va(&vmcs_guest_)) == vmx::error_code::success)
  {
    NOTHING;
  }
  else
  {
    error();
  }
}

void vcpu::setup_vmcs_host() noexcept
{
  auto gdtr = read<gdtr_t>();

  host_gdtr(gdtr);
  host_idtr(read<idtr_t>());
  host_cs(seg_t{ gdtr, read<cs_t>() });
  host_ds(seg_t{ gdtr, read<ds_t>() });
  host_es(seg_t{ gdtr, read<es_t>() });
  host_fs(seg_t{ gdtr, read<fs_t>() });
  host_gs(seg_t{ gdtr, read<gs_t>() });
  host_ss(seg_t{ gdtr, read<ss_t>() });
  host_tr(seg_t{ gdtr, read<tr_t>() });

  host_cr0(read<cr0_t>());
  host_cr3(read<cr3_t>());
  host_cr4(read<cr4_t>());

  host_rsp(reinterpret_cast<uint64_t>(std::end(stack_)));
  host_rip(reinterpret_cast<uint64_t>(&vcpu::entry_host_));
}

void vcpu::setup_vmcs_guest() noexcept
{
  vmx::vmwrite(vmx::vmcs::field::ctrl_virtual_processor_identifier, 1);
  vmx::vmwrite(vmx::vmcs::field::ctrl_ept_pointer, ept_.ept_pointer());

  //
  // VMCS link pointer points to the shadow VMCS if VMCS shadowing is enabled.
  // If VMCS shadowing is disabled, intel advises to set this value to 0xFFFFFFFFFFFFFFFF.
  //
  vmx::vmwrite(vmx::vmcs::field::guest_vmcs_link_pointer, ~0ull);

  pin_based_controls(msr::vmx_pinbased_ctls{ 0 });

  msr::vmx_procbased_ctls procbased_ctls{ 0 };
  procbased_ctls.activate_secondary_controls = true;
  procbased_ctls.use_msr_bitmaps = true;
  procbased_ctls.rdtsc_exiting = true;
  procbased_ctls.use_io_bitmaps = true;
  //procbased_ctls.mov_dr_exiting = true;
  processor_based_controls(procbased_ctls);

  msr::vmx_procbased_ctls2 procbased_ctls2{ 0 };
  procbased_ctls2.enable_ept = true;
  procbased_ctls2.enable_vpid = true;
  processor_based_controls2(procbased_ctls2);

  msr::vmx_entry_ctls entry_ctls{ 0 };
  entry_ctls.ia32e_mode_guest = true;
  vm_entry_controls(entry_ctls);

  msr::vmx_exit_ctls exit_ctls{ 0 };
  exit_ctls.ia32e_mode_host = true;
  vm_exit_controls(exit_ctls);

  msr_bitmap(vmx::msr_bitmap{ 0 });

  vmx::io_bitmap io_bmp{ 0 };
  memset(&io_bmp.data, 0xff, sizeof(io_bmp));
  //bitmap(io_bmp.data).clear(0x5658);

  io_bitmap(io_bmp);

//   vmx::exception_bitmap expt_bitmap{ 0 };
//   expt_bitmap.page_fault = true;
//   exception_bitmap(expt_bitmap);

  //////////////////////////////////////////////////////////////////////////

  auto gdtr = read<gdtr_t>();
  guest_gdtr(gdtr);
  guest_idtr(read<idtr_t>());
  guest_cs(seg_t{ gdtr, read<cs_t>() });
  guest_ds(seg_t{ gdtr, read<ds_t>() });
  guest_es(seg_t{ gdtr, read<es_t>() });
  guest_fs(seg_t{ gdtr, read<fs_t>() });
  guest_gs(seg_t{ gdtr, read<gs_t>() });
  guest_ss(seg_t{ gdtr, read<ss_t>() });
  guest_tr(seg_t{ gdtr, read<tr_t>() });
  guest_ldtr(seg_t{ gdtr, read<ldtr_t>() });

  guest_cr0(read<cr0_t>());
  guest_cr3(read<cr3_t>());
  guest_cr4(read<cr4_t>());

  guest_cr0_shadow(read<cr0_t>());
  guest_cr4_shadow(read<cr4_t>());

  guest_debugctl(msr::read<msr::debugctl>());
  guest_dr7(read<dr7_t>());
  guest_rflags(read<rflags_t>());

  guest_rsp(reinterpret_cast<uint64_t>(std::end(stack_)));
  guest_rip(reinterpret_cast<uint64_t>(&vcpu::entry_guest_));
}

void vcpu::entry_host() noexcept
{
  suppress_rip_adjust_ = false;

  auto saved_rsp    = exit_context_.rsp;
  auto saved_rflags = exit_context_.rflags;

  {
    exit_context_.rsp    = guest_rsp();
    exit_context_.rip    = guest_rip();
    exit_context_.rflags = guest_rflags();

    {
      exit_handler_->handle(this);

      if (state_ == vcpu_state::terminated)
      {
        return;
      }

      if (!suppress_rip_adjust_)
      {
        exit_context_.rip += exit_instruction_length();
      }
    }

    guest_rsp(exit_context_.rsp);
    guest_rip(exit_context_.rip);
    guest_rflags(exit_context_.rflags);
  }

  exit_context_.rflags = saved_rflags;
  exit_context_.rsp    = saved_rsp;
  exit_context_.rip    = reinterpret_cast<uint64_t>(&vmx::vmresume);
}

void vcpu::entry_guest() noexcept
{
  guest_context_.rax = static_cast<uint64_t>(vcpu_state::launching);
}

}
