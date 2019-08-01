// #define HVPP_ENABLE_INTERRUPT_TRACE

#if defined(HVPP_ENABLE_INTERRUPT_TRACE)
# define __trace_interrupt_inject_vectoring()                                 \
  hvpp_trace("interrupt_inject (vectoring) - vector: %s, type: %s, nmi_unblocking: %u, valid: %u",\
             ::ia32::to_string(vectoring.vector()),                           \
             ::ia32::vmx::to_string(vectoring.type()),                        \
             vectoring.nmi_unblocking(),                                      \
             vectoring.valid())

# define __trace_interrupt_inject_push()                                      \
  hvpp_trace("interrupt_inject (push-%s) - vector: %s, type: %s, queue_size: %u",\
              front ? "front" : "back",                                       \
              ::ia32::to_string(interrupt.vector()),                          \
              ::ia32::vmx::to_string(interrupt.type()),                       \
              queue.size() + 1)

# define __trace_interrupt_inject_pop()                                       \
  hvpp_trace("interrupt_inject (pop-front) - vector: %s, type: %s, queue_size: %u",\
             ::ia32::to_string(queue.front().vector()),                       \
             ::ia32::vmx::to_string(queue.front().type()),                    \
             queue.size() - 1)


# define __trace_interrupt_inject()                                           \
  hvpp_trace("interrupt_inject - vector: %s, type: %s",                       \
             ::ia32::to_string(interrupt.vector()),                           \
             ::ia32::vmx::to_string(interrupt.type()))


#else
# define __trace_interrupt_inject_vectoring()
# define __trace_interrupt_inject_push()
# define __trace_interrupt_inject_pop()
# define __trace_interrupt_inject()
#endif


namespace hvpp {

auto vcpu_t::stacked_lock_guard(spinlock& lock) noexcept -> stacked_lock_guard_t
{
  return stacked_lock_guard_t{ *this, lock };
}

void vcpu_t::stacked_lock_guard_push(spinlock& lock) noexcept
{
  lock.lock();
  spinlock_queue_.push_back(&lock);
}

void vcpu_t::stacked_lock_guard_pop() noexcept
{
  auto& lock = *spinlock_queue_.back();
  spinlock_queue_.pop_back();

  lock.unlock();
}

vcpu_t::stacked_lock_guard_t::stacked_lock_guard_t(vcpu_t& vp, spinlock& lock) noexcept
  : vp_{ vp }
{
  vp_.stacked_lock_guard_push(lock);
}

vcpu_t::stacked_lock_guard_t::~stacked_lock_guard_t() noexcept
{
  vp_.stacked_lock_guard_pop();
}

auto vcpu_t::interrupt_info() const noexcept -> interrupt_t
{
  interrupt_t result;
  result.info_ = exit_interruption_info();

  if (result.info_.valid)
  {
    if (result.info_.error_code_valid)
    {
      result.error_code_ = exit_interruption_error_code();
    }

    result.rip_adjust_ = exit_instruction_length();
  }

  return result;
}

auto vcpu_t::idt_vectoring_info() const noexcept -> interrupt_t
{
  interrupt_t result;
  result.info_ = exit_idt_vectoring_info();

  if (result.info_.valid)
  {
    if (result.info_.error_code_valid)
    {
      result.error_code_ = exit_idt_vectoring_error_code();
    }

    result.rip_adjust_ = exit_instruction_length();
  }

  return result;
}

#define interrupt_type_to_queue_type(type)  (                         \
    type == vmx::interrupt_type::external ? interrupt_queue_external  \
  : type == vmx::interrupt_type::nmi      ? interrupt_queue_nmi       \
  : vcpu_t::interrupt_queue_type(-1) /* error */                      \
  )

bool vcpu_t::interrupt_inject(interrupt_t interrupt, bool front /*= false */) noexcept
{
  bool interruptible = true;

  if (interrupt.type() == vmx::interrupt_type::external)
  {
    //
    // External interrupts cannot be injected into the
    // guest if guest isn't interruptible (e.g.: guest
    // is blocked by "mov ss", or EFLAGS.IF == 0).
    //
    interruptible = context().rflags.interrupt_enable_flag &&
                    guest_interruptibility_state().flags;
  }
  else if (interrupt.type() == vmx::interrupt_type::nmi)
  {
    interruptible = !guest_interruptibility_state().blocking_by_nmi;
  }

  if (auto vectoring = idt_vectoring_info();
           vectoring.valid())
  {
    __trace_interrupt_inject_vectoring();
  }

  if (interruptible)
  {
    //
    // Inject interrupt immediately.
    //
    interrupt_inject_force(interrupt);

    //
    // Signalize immediately injected interrupt.
    //
    return true;
  }

  auto  queue_type = interrupt_type_to_queue_type(interrupt.type());
  auto& queue      = pending_interrupt_queue_[queue_type];

  //
  // Inform that there was an attempt to inject an interrupt
  // while CPU wasn't in the interruptible state.
  //
  __trace_interrupt_inject_push();

  //
  // Enqueue pending interrupt.
  //
  front
    ? queue.push_front(std::move(interrupt))
    : queue.push_back(std::move(interrupt));

  //
  // Enable Interrupt/NMI-window exiting.
  //
  auto procbased_ctls = processor_based_controls();

  if (interrupt.type() == vmx::interrupt_type::external)
  {
    procbased_ctls.interrupt_window_exiting = true;
  }
  else if (interrupt.type() == vmx::interrupt_type::nmi)
  {
    procbased_ctls.nmi_window_exiting = true;
  }

  processor_based_controls(procbased_ctls);

  //
  // "false" signalizes that the interrupt hasn't been
  // immediately injected.
  //
  return false;
}

void vcpu_t::interrupt_inject_force(interrupt_t interrupt) noexcept
{
  //
  // Inform that interrupt is being injected.
  //
  __trace_interrupt_inject();

  //
  // Set ctrl_vmentry_interruption_info.
  //
  entry_interruption_info(interrupt.info_);

  if (interrupt.valid())
  {
    //
    // These hardware exceptions must provide an error code:
    //  - #DF (8)  - always 0
    //  - #TS (10)
    //  - #NP (11)
    //  - #SS (12)
    //  - #GP (13)
    //  - #PF (14)
    //  - #AC (17) - always 0
    //
    // (ref: Vol3A[6.3.1(External Interrupts)])
    //

    if (interrupt.type() == vmx::interrupt_type::hardware_exception)
    {
      switch (interrupt.vector())
      {
        case exception_vector::invalid_tss:
        case exception_vector::segment_not_present:
        case exception_vector::stack_segment_fault:
        case exception_vector::general_protection:
        case exception_vector::page_fault:
          hvpp_assert(interrupt.error_code_valid());
          entry_interruption_error_code(interrupt.error_code());
          break;

        case exception_vector::double_fault:
        case exception_vector::alignment_check:
          hvpp_assert(interrupt.error_code_valid() && interrupt.error_code().flags == 0);
          entry_interruption_error_code(interrupt.error_code());
          break;

        default:
          break;
      }
    }

    //
    // The instruction pointer that is pushed on the stack depends
    // on the type of event and whether nested exceptions occur during
    // its delivery.  The term current guest RIP refers to the value
    // to be loaded from the guest-state area.  The value pushed is
    // determined as follows:
    //
    //  - If VM entry successfully injects (with no nested exception)
    //    an event with interruption type external interrupt, NMI, or
    //    hardware exception, the current guest RIP is pushed on the stack.
    //
    //  - If VM entry successfully injects (with no nested exception)
    //    an event with interruption type software interrupt, privileged
    //    software exception, or software exception, the current guest RIP
    //    is incremented by the VM-entry instruction length before being
    //    pushed on the stack.
    //
    //  - If VM entry encounters an exception while injecting an event
    //    and that exception does not cause a VM exit, the current guest
    //    RIP is pushed on the stack regardless of event type or VM-entry
    //    instruction length.  If the encountered exception does cause a
    //    VM exit that saves RIP, the saved RIP is current guest RIP.
    //
    // (ref: Vol3C[26.5.1.1(Details of Vectored-Event Injection)])
    //

    switch (interrupt.type())
    {
      case vmx::interrupt_type::external:
      case vmx::interrupt_type::nmi:
      case vmx::interrupt_type::hardware_exception:
      case vmx::interrupt_type::other_event:
      default:
        break;

      case vmx::interrupt_type::software:
      case vmx::interrupt_type::privileged_exception:
      case vmx::interrupt_type::software_exception:
        if (interrupt.rip_adjust_ == -1)
        {
          interrupt.rip_adjust_ = static_cast<int>(exit_instruction_length());
        }

        if (interrupt.rip_adjust_ > 0)
        {
          entry_instruction_length(interrupt.rip_adjust_);
        }
        break;
    }
  }
}

void vcpu_t::interrupt_inject_pending(interrupt_queue_type queue_type) noexcept
{
  //
  // Dequeue and inject pending interrupt.
  //
  auto& queue = pending_interrupt_queue_[queue_type];

  __trace_interrupt_inject_pop();

  interrupt_inject_force(queue.front());
  queue.pop_front();
}

bool vcpu_t::interrupt_is_pending(interrupt_queue_type queue_type) const noexcept
{
  auto& queue = pending_interrupt_queue_[queue_type];

  return queue.size() > 0;
}

auto vcpu_t::exit_instruction_info_guest_va() const noexcept -> void*
{
  auto instruction_info = exit_instruction_info().common;
  auto displacement = exit_qualification().displacement;

  auto base = !instruction_info.base_register_invalid
    ? context_.gp_register[instruction_info.base_register]
    : 0;

  auto index = !instruction_info.index_register_invalid
    ? context_.gp_register[instruction_info.index_register]
    : 0;

  auto segment_base = reinterpret_cast<uint64_t>(
    guest_segment_base_address(instruction_info.segment_register));

  auto result = segment_base;
  result += base;
  result += index;
  result += displacement;
  result &= vmx::instruction_info_t::size_to_mask[instruction_info.address_size];

  return reinterpret_cast<void*>(result);
}

//
// control state
//

auto vcpu_t::vcpu_id() const noexcept -> uint16_t
{
  uint16_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_virtual_processor_identifier, result);
  return result;
}

void vcpu_t::vcpu_id(uint16_t virtual_processor_identifier) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_virtual_processor_identifier, virtual_processor_identifier);
}

auto vcpu_t::ept_pointer() const noexcept -> ept_ptr_t
{
  ept_ptr_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_ept_pointer, result);
  return result;
}

void vcpu_t::ept_pointer(ept_ptr_t ept_pointer) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_ept_pointer, ept_pointer);
}

auto vcpu_t::vmcs_link_pointer() const noexcept -> pa_t
{
  pa_t result;
  vmx::vmread(vmx::vmcs_t::field::guest_vmcs_link_pointer, result);
  return result;
}

void vcpu_t::vmcs_link_pointer(pa_t link_pointer) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_vmcs_link_pointer, link_pointer);
}

auto vcpu_t::pin_based_controls() const noexcept -> msr::vmx_pinbased_ctls_t
{
  msr::vmx_pinbased_ctls_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_pin_based_vm_execution_controls, result);
  return result;
}

void vcpu_t::pin_based_controls(msr::vmx_pinbased_ctls_t controls) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_pin_based_vm_execution_controls, vmx::adjust(controls));
}

auto vcpu_t::processor_based_controls() const noexcept -> msr::vmx_procbased_ctls_t
{
  msr::vmx_procbased_ctls_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_processor_based_vm_execution_controls, result);
  return result;
}

void vcpu_t::processor_based_controls(msr::vmx_procbased_ctls_t controls) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_processor_based_vm_execution_controls, vmx::adjust(controls));
}

auto vcpu_t::processor_based_controls2() const noexcept -> msr::vmx_procbased_ctls2_t
{
  msr::vmx_procbased_ctls2_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_secondary_processor_based_vm_execution_controls, result);
  return result;
}

void vcpu_t::processor_based_controls2(msr::vmx_procbased_ctls2_t controls) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_secondary_processor_based_vm_execution_controls, vmx::adjust(controls));
}

auto vcpu_t::vm_entry_controls() const noexcept -> msr::vmx_entry_ctls_t
{
  msr::vmx_entry_ctls_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_vmentry_controls, result);
  return result;
}

void vcpu_t::vm_entry_controls(msr::vmx_entry_ctls_t controls) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_vmentry_controls, vmx::adjust(controls));
}

auto vcpu_t::vm_exit_controls() const noexcept -> msr::vmx_exit_ctls_t
{
  msr::vmx_exit_ctls_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_vmexit_controls, result);
  return result;
}

void vcpu_t::vm_exit_controls(msr::vmx_exit_ctls_t controls) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_vmexit_controls, vmx::adjust(controls));
}

auto vcpu_t::exception_bitmap() const noexcept -> vmx::exception_bitmap_t
{
  vmx::exception_bitmap_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_exception_bitmap, result);
  return result;
}

void vcpu_t::exception_bitmap(vmx::exception_bitmap_t exception_bitmap) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_exception_bitmap, exception_bitmap);
}

auto vcpu_t::msr_bitmap() const noexcept -> const vmx::msr_bitmap_t&
{
  return msr_bitmap_;
}

void vcpu_t::msr_bitmap(const vmx::msr_bitmap_t& msr_bitmap) noexcept
{
  msr_bitmap_ = msr_bitmap;
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_msr_bitmap_address, pa_t::from_va(msr_bitmap_.data));
}

auto vcpu_t::io_bitmap() const noexcept -> const vmx::io_bitmap_t&
{
  return io_bitmap_;
}

void vcpu_t::io_bitmap(const vmx::io_bitmap_t& io_bitmap) noexcept
{
  io_bitmap_ = io_bitmap;
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_io_bitmap_a_address, pa_t::from_va(io_bitmap_.a));
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_io_bitmap_b_address, pa_t::from_va(io_bitmap_.b));
}

auto vcpu_t::pagefault_error_code_mask() const noexcept -> pagefault_error_code_t
{
  pagefault_error_code_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_pagefault_error_code_mask, result);
  return result;
}

void vcpu_t::pagefault_error_code_mask(pagefault_error_code_t mask) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_pagefault_error_code_mask, mask);
}

auto vcpu_t::pagefault_error_code_match() const noexcept -> pagefault_error_code_t
{
  pagefault_error_code_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_pagefault_error_code_match, result);
  return result;
}

void vcpu_t::pagefault_error_code_match(pagefault_error_code_t match) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_pagefault_error_code_match, match);
}

//
// control entry state
//

auto vcpu_t::cr0_guest_host_mask() const noexcept -> cr0_t
{
  cr0_t cr0;
  vmx::vmread(vmx::vmcs_t::field::ctrl_cr0_guest_host_mask, cr0);
  return cr0;
}

void vcpu_t::cr0_guest_host_mask(cr0_t cr0) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_cr0_guest_host_mask, cr0);
}

auto vcpu_t::cr0_shadow() const noexcept -> cr0_t
{
  cr0_t cr0;
  vmx::vmread(vmx::vmcs_t::field::ctrl_cr0_read_shadow, cr0);
  return cr0;
}

void vcpu_t::cr0_shadow(cr0_t cr0) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_cr0_read_shadow, cr0);
}

auto vcpu_t::cr4_guest_host_mask() const noexcept -> cr4_t
{
  cr4_t cr4;
  vmx::vmread(vmx::vmcs_t::field::ctrl_cr4_guest_host_mask, cr4);
  return cr4;
}

void vcpu_t::cr4_guest_host_mask(cr4_t cr4) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_cr4_guest_host_mask, cr4);
}

auto vcpu_t::cr4_shadow() const noexcept -> cr4_t
{
  cr4_t cr4;
  vmx::vmread(vmx::vmcs_t::field::ctrl_cr4_read_shadow, cr4);
  return cr4;
}

void vcpu_t::cr4_shadow(cr4_t cr4) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_cr4_read_shadow, cr4);
}

auto vcpu_t::entry_instruction_length() const noexcept -> uint32_t
{
  uint32_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_vmentry_instruction_length, result);
  return result;
}

void vcpu_t::entry_instruction_length(uint32_t instruction_length) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_vmentry_instruction_length, instruction_length);
}

auto vcpu_t::entry_interruption_info() const noexcept -> vmx::interrupt_info_t
{
  vmx::interrupt_info_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_vmentry_interruption_info, result);
  return result;
}

void vcpu_t::entry_interruption_info(vmx::interrupt_info_t info) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_vmentry_interruption_info, info);
}

auto vcpu_t::entry_interruption_error_code() const noexcept -> exception_error_code_t
{
  exception_error_code_t result;
  vmx::vmread(vmx::vmcs_t::field::ctrl_vmentry_exception_error_code, result);
  return result;
}

void vcpu_t::entry_interruption_error_code(exception_error_code_t error_code) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::ctrl_vmentry_exception_error_code, error_code);
}

//
// exit state
//

auto vcpu_t::exit_instruction_error() const noexcept -> vmx::instruction_error
{
  vmx::instruction_error result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_instruction_error, result);
  return result;
}

auto vcpu_t::exit_instruction_info() const noexcept -> vmx::instruction_info_t
{
  vmx::instruction_info_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_instruction_info, result);
  return result;
}

auto vcpu_t::exit_instruction_length() const noexcept -> uint32_t
{
  uint32_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_instruction_length, result);
  return result;
}

auto vcpu_t::exit_interruption_info() const noexcept -> vmx::interrupt_info_t
{
  vmx::interrupt_info_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_interruption_info, result);
  return result;
}

auto vcpu_t::exit_interruption_error_code() const noexcept -> exception_error_code_t
{
  exception_error_code_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_interruption_error_code, result);
  return result;
}

auto vcpu_t::exit_idt_vectoring_info() const noexcept -> vmx::interrupt_info_t
{
  vmx::interrupt_info_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_idt_vectoring_info, result);
  return result;
}

auto vcpu_t::exit_idt_vectoring_error_code() const noexcept -> exception_error_code_t
{
  exception_error_code_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_idt_vectoring_error_code, result);
  return result;
}

auto vcpu_t::exit_reason() const noexcept -> vmx::exit_reason
{
  vmx::exit_reason result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_reason, result);
  return result;
}

auto vcpu_t::exit_qualification() const noexcept -> vmx::exit_qualification_t
{
  vmx::exit_qualification_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_qualification, result);
  return result;
}

auto vcpu_t::exit_guest_physical_address() const noexcept -> pa_t
{
  pa_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_guest_physical_address, result);
  return result;
}

auto vcpu_t::exit_guest_linear_address() const noexcept -> va_t
{
  va_t result;
  vmx::vmread(vmx::vmcs_t::field::vmexit_guest_linear_address, result);
  return result;
}

//
// guest state
//

auto vcpu_t::guest_cr0() const noexcept -> cr0_t
{
  cr0_t cr0;
  vmx::vmread(vmx::vmcs_t::field::guest_cr0, cr0);
  return cr0;
}

void vcpu_t::guest_cr0(cr0_t cr0) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_cr0, cr0);
}

auto vcpu_t::guest_cr3() const noexcept -> cr3_t
{
  cr3_t cr3;
  vmx::vmread(vmx::vmcs_t::field::guest_cr3, cr3);
  return cr3;
}

void vcpu_t::guest_cr3(cr3_t cr3) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_cr3, cr3);
}

auto vcpu_t::guest_cr4() const noexcept -> cr4_t
{
  cr4_t cr4;
  vmx::vmread(vmx::vmcs_t::field::guest_cr4, cr4);
  return cr4;
}

void vcpu_t::guest_cr4(cr4_t cr4) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_cr4, cr4);
}

auto vcpu_t::guest_dr7() const noexcept -> dr7_t
{
  dr7_t dr7;
  vmx::vmread(vmx::vmcs_t::field::guest_dr7, dr7);

  return dr7;
}

void vcpu_t::guest_dr7(dr7_t dr7) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_dr7, dr7);
}

auto vcpu_t::guest_debugctl() const noexcept -> msr::debugctl_t
{
  msr::debugctl_t debugctl;
  vmx::vmread(vmx::vmcs_t::field::guest_debugctl, debugctl);
  return debugctl;
}

void vcpu_t::guest_debugctl(msr::debugctl_t debugctl) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_debugctl, debugctl);
}

auto vcpu_t::guest_rsp() const noexcept -> uint64_t
{
  uint64_t rsp;
  vmx::vmread(vmx::vmcs_t::field::guest_rsp, rsp);
  return rsp;
}

void vcpu_t::guest_rsp(uint64_t rsp) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_rsp, rsp);
}

auto vcpu_t::guest_rip() const noexcept -> uint64_t
{
  uint64_t rip;
  vmx::vmread(vmx::vmcs_t::field::guest_rip, rip);
  return rip;
}

void vcpu_t::guest_rip(uint64_t rip) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_rip, rip);
}

auto vcpu_t::guest_rflags() const noexcept -> rflags_t
{
  rflags_t rflags;
  vmx::vmread(vmx::vmcs_t::field::guest_rflags, rflags);
  return rflags;
}

void vcpu_t::guest_rflags(rflags_t rflags) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_rflags, rflags);
}

auto vcpu_t::guest_gdtr() const noexcept -> gdtr_t
{
  gdtr_t gdtr;
  vmx::vmread(vmx::vmcs_t::field::guest_gdtr_base, gdtr.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_gdtr_limit, gdtr.limit);
  return gdtr;
}

void vcpu_t::guest_gdtr(gdtr_t gdtr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_gdtr_base, gdtr.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_gdtr_limit, gdtr.limit);
}

auto vcpu_t::guest_idtr() const noexcept -> idtr_t
{
  idtr_t idtr;
  vmx::vmread(vmx::vmcs_t::field::guest_idtr_base, idtr.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_idtr_limit, idtr.limit);
  return idtr;
}

void vcpu_t::guest_idtr(idtr_t idtr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_idtr_base, idtr.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_idtr_limit, idtr.limit);
}

auto vcpu_t::guest_cs() const noexcept -> segment_t<cs_t>
{
  segment_t<cs_t> cs;
  vmx::vmread(vmx::vmcs_t::field::guest_cs_base, cs.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_cs_limit, cs.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_cs_access_rights, cs.access);
  vmx::vmread(vmx::vmcs_t::field::guest_cs_selector, cs.selector);
  return cs;
}

void vcpu_t::guest_cs(segment_t<cs_t> cs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_cs_base, cs.base_address /* 0 */);
  vmx::vmwrite(vmx::vmcs_t::field::guest_cs_limit, cs.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_cs_access_rights, cs.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_cs_selector, cs.selector);
}

auto vcpu_t::guest_ds() const noexcept -> segment_t<ds_t>
{
  segment_t<ds_t> ds;
  vmx::vmread(vmx::vmcs_t::field::guest_ds_base, ds.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_ds_limit, ds.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_ds_access_rights, ds.access);
  vmx::vmread(vmx::vmcs_t::field::guest_ds_selector, ds.selector);

  return ds;
}

void vcpu_t::guest_ds(segment_t<ds_t> ds) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_ds_base, ds.base_address /* 0 */);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ds_limit, ds.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ds_access_rights, ds.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ds_selector, ds.selector);
}

auto vcpu_t::guest_es() const noexcept -> segment_t<es_t>
{
  segment_t<es_t> es;
  vmx::vmread(vmx::vmcs_t::field::guest_es_base, es.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_es_limit, es.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_es_access_rights, es.access);
  vmx::vmread(vmx::vmcs_t::field::guest_es_selector, es.selector);

  return es;
}

void vcpu_t::guest_es(segment_t<es_t> es) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_base, es.base_address /* 0 */);
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_limit, es.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_access_rights, es.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_selector, es.selector);
}

auto vcpu_t::guest_fs() const noexcept -> segment_t<fs_t>
{
  segment_t<fs_t> fs;
  vmx::vmread(vmx::vmcs_t::field::guest_fs_base, fs.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_fs_limit, fs.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_fs_access_rights, fs.access);
  vmx::vmread(vmx::vmcs_t::field::guest_fs_selector, fs.selector);

  return fs;
}

void vcpu_t::guest_fs(segment_t<fs_t> fs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_fs_base, fs.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_fs_limit, fs.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_fs_access_rights, fs.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_fs_selector, fs.selector);
}

auto vcpu_t::guest_gs() const noexcept -> segment_t<gs_t>
{
  segment_t<gs_t> gs;
  vmx::vmread(vmx::vmcs_t::field::guest_gs_base, gs.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_gs_limit, gs.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_gs_access_rights, gs.access);
  vmx::vmread(vmx::vmcs_t::field::guest_gs_selector, gs.selector);

  return gs;
}

void vcpu_t::guest_gs(segment_t<gs_t> gs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_gs_base, gs.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_gs_limit, gs.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_gs_access_rights, gs.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_gs_selector, gs.selector);
}

auto vcpu_t::guest_ss() const noexcept -> segment_t<ss_t>
{
  segment_t<ss_t> ss;
  vmx::vmread(vmx::vmcs_t::field::guest_ss_base, ss.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_ss_limit, ss.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_ss_access_rights, ss.access);
  vmx::vmread(vmx::vmcs_t::field::guest_ss_selector, ss.selector);

  return ss;
}

void vcpu_t::guest_ss(segment_t<ss_t> ss) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_ss_base, ss.base_address /* 0 */);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ss_limit, ss.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ss_access_rights, ss.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ss_selector, ss.selector);
}

auto vcpu_t::guest_tr() const noexcept -> segment_t<tr_t>
{
  segment_t<tr_t> tr;
  vmx::vmread(vmx::vmcs_t::field::guest_tr_base, tr.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_tr_limit, tr.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_tr_access_rights, tr.access);
  vmx::vmread(vmx::vmcs_t::field::guest_tr_selector, tr.selector);

  return tr;
}

void vcpu_t::guest_tr(segment_t<tr_t> tr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_tr_base, tr.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_tr_limit, tr.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_tr_access_rights, tr.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_tr_selector, tr.selector);
}

auto vcpu_t::guest_ldtr() const noexcept -> segment_t<ldtr_t>
{
  segment_t<ldtr_t> ldtr;
  vmx::vmread(vmx::vmcs_t::field::guest_ldtr_base, ldtr.base_address);
  vmx::vmread(vmx::vmcs_t::field::guest_ldtr_limit, ldtr.limit);
  vmx::vmread(vmx::vmcs_t::field::guest_ldtr_access_rights, ldtr.access);
  vmx::vmread(vmx::vmcs_t::field::guest_ldtr_selector, ldtr.selector);

  return ldtr;
}

void vcpu_t::guest_ldtr(segment_t<ldtr_t> ldtr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_ldtr_base, ldtr.base_address);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ldtr_limit, ldtr.limit);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ldtr_access_rights, ldtr.access);
  vmx::vmwrite(vmx::vmcs_t::field::guest_ldtr_selector, ldtr.selector);
}

auto vcpu_t::guest_segment_base_address(int index) const noexcept -> void*
{
  void* result;
  vmx::vmread(vmx::vmcs_t::field::guest_es_base + (index << 1), result);
  return result;
}

void vcpu_t::guest_segment_base_address(int index, void* base_address) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_base + (index << 1), base_address);
}

auto vcpu_t::guest_segment_limit(int index) const noexcept -> uint32_t
{
  uint32_t result;
  vmx::vmread(vmx::vmcs_t::field::guest_es_limit + (index << 1), result);
  return result;
}

void vcpu_t::guest_segment_limit(int index, uint32_t limit) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_limit + (index << 1), limit);
}

auto vcpu_t::guest_segment_access(int index) const noexcept -> segment_access_vmx_t
{
  segment_access_vmx_t result;
  vmx::vmread(vmx::vmcs_t::field::guest_es_access_rights + (index << 1), result);
  return result;
}

void vcpu_t::guest_segment_access(int index, segment_access_vmx_t access_rights) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_access_rights + (index << 1), access_rights);
}

auto vcpu_t::guest_segment_selector(int index) const noexcept -> segment_selector_t
{
  segment_selector_t result;
  vmx::vmread(vmx::vmcs_t::field::guest_es_selector + (index << 1), result);
  return result;
}

void vcpu_t::guest_segment_selector(int index, segment_selector_t selector) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_es_selector + (index << 1), selector);
}

auto vcpu_t::guest_segment(int index) const noexcept -> segment_t<>
{
  hvpp_assert(index >= context_t::seg_min && index <= context_t::seg_max);

  return segment_t<> {
    guest_segment_base_address(index),
    guest_segment_limit(index),
    guest_segment_access(index),
    guest_segment_selector(index),
  };
}

void vcpu_t::guest_segment(int index, segment_t<> seg) noexcept
{
  hvpp_assert(index >= context_t::seg_min && index <= context_t::seg_max);

  guest_segment_base_address(index, seg.base_address);
  guest_segment_limit(index, seg.limit);
  guest_segment_access(index, seg.access);
  guest_segment_selector(index, seg.selector);
}

auto vcpu_t::guest_interruptibility_state() const noexcept -> vmx::interruptibility_state_t
{
  vmx::interruptibility_state_t interruptibility_state;
  vmx::vmread(vmx::vmcs_t::field::guest_interruptibility_state, interruptibility_state);
  return interruptibility_state;
}

void vcpu_t::guest_interruptibility_state(vmx::interruptibility_state_t interruptibility_state) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::guest_interruptibility_state, interruptibility_state);
}

//
// host state
//

auto vcpu_t::host_cr0() const noexcept -> cr0_t
{
  cr0_t cr0;
  vmx::vmread(vmx::vmcs_t::field::host_cr0, cr0);
  return cr0;
}

void vcpu_t::host_cr0(cr0_t cr0) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_cr0, cr0);
}

auto vcpu_t::host_cr3() const noexcept -> cr3_t
{
  cr3_t cr3;
  vmx::vmread(vmx::vmcs_t::field::host_cr3, cr3);
  return cr3;
}

void vcpu_t::host_cr3(cr3_t cr3) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_cr3, cr3);
}

auto vcpu_t::host_cr4() const noexcept -> cr4_t
{
  cr4_t cr4;
  vmx::vmread(vmx::vmcs_t::field::host_cr4, cr4);
  return cr4;
}

void vcpu_t::host_cr4(cr4_t cr4) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_cr4, cr4);
}

auto vcpu_t::host_rsp() const noexcept -> uint64_t
{
  uint64_t rsp;
  vmx::vmread(vmx::vmcs_t::field::host_rsp, rsp);
  return rsp;
}

void vcpu_t::host_rsp(uint64_t rsp) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_rsp, rsp);
}

auto vcpu_t::host_rip() const noexcept -> uint64_t
{
  uint64_t rip;
  vmx::vmread(vmx::vmcs_t::field::host_rip, rip);
  return rip;
}

void vcpu_t::host_rip(uint64_t rip) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_rip, rip);
}

//
// The base addresses for GDTR and IDTR are loaded from the
// GDTR base-address field and the IDTR base-address field,
// respectively.
// If the processor supports the Intel 64 architecture and
// the processor supports N < 64 linear address bits, each
// of bits 63:N of each base address is set to the value of
// bit N–1 of that base address.
// The GDTR and IDTR limits are each set to FFFFH.
// (ref: Vol3C[27.5.2(Loading Host Segment and Descriptor-Table Registers)])
//

auto vcpu_t::host_gdtr() const noexcept -> gdtr_t
{
  gdtr_t gdtr;
  vmx::vmread(vmx::vmcs_t::field::host_gdtr_base, gdtr.base_address);
  gdtr.limit = 0xffff;
  return gdtr;
}

void vcpu_t::host_gdtr(gdtr_t gdtr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_gdtr_base, gdtr.base_address);
}

auto vcpu_t::host_idtr() const noexcept -> idtr_t
{
  idtr_t idtr;
  vmx::vmread(vmx::vmcs_t::field::host_idtr_base, idtr.base_address);
  idtr.limit = 0xffff;
  return idtr;
}

void vcpu_t::host_idtr(idtr_t idtr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_idtr_base, idtr.base_address);
}

//
// Index - Selects one of 8192 descriptors in the GDT or LDT.
// The processor multiplies the index value by 8 (the number
// of bytes in a segment descriptor) and adds the result to the
// base address of the GDT or LDT (from the GDTR or LDTR register,
// respectively).
// (ref: Vol3A[3.4.2(Segment Selectors)])
//
// Note that
//   (selector.index * 8)      could be represented by
//   (selector.index << 3)     or in this case even by
//   (selector.flags & 7)      or
//   (selector.flags & 0b0111)
//

auto vcpu_t::host_cs() const noexcept -> segment_t<cs_t>
{
  segment_t<cs_t> cs;
  vmx::vmread(vmx::vmcs_t::field::host_cs_selector, cs.selector);
  return cs;
}

void vcpu_t::host_cs(segment_t<cs_t> cs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_cs_selector, cs.selector.index * 8);
}

auto vcpu_t::host_ds() const noexcept -> segment_t<ds_t>
{
  segment_t<ds_t> ds;
  vmx::vmread(vmx::vmcs_t::field::host_ds_selector, ds.selector);
  return ds;
}

void vcpu_t::host_ds(segment_t<ds_t> ds) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_ds_selector, ds.selector.index * 8);
}

auto vcpu_t::host_es() const noexcept -> segment_t<es_t>
{
  segment_t<es_t> es;
  vmx::vmread(vmx::vmcs_t::field::host_es_selector, es.selector);
  return es;
}

void vcpu_t::host_es(segment_t<es_t> es) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_es_selector, es.selector.index * 8);
}

auto vcpu_t::host_fs() const noexcept -> segment_t<fs_t>
{
  segment_t<fs_t> fs;
  vmx::vmread(vmx::vmcs_t::field::host_fs_selector, fs.selector);
  vmx::vmread(vmx::vmcs_t::field::host_fs_base, fs.base_address);
  return fs;
}

void vcpu_t::host_fs(segment_t<fs_t> fs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_fs_selector, fs.selector.index * 8);
  vmx::vmwrite(vmx::vmcs_t::field::host_fs_base, fs.base_address);
}

auto vcpu_t::host_gs() const noexcept -> segment_t<gs_t>
{
  segment_t<gs_t> gs;
  vmx::vmread(vmx::vmcs_t::field::host_gs_selector, gs.selector);
  vmx::vmread(vmx::vmcs_t::field::host_gs_base, gs.base_address);
  return gs;
}

void vcpu_t::host_gs(segment_t<gs_t> gs) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_gs_selector, gs.selector.index * 8);
  vmx::vmwrite(vmx::vmcs_t::field::host_gs_base, gs.base_address);
}

auto vcpu_t::host_ss() const noexcept -> segment_t<ss_t>
{
  segment_t<ss_t> ss;
  vmx::vmread(vmx::vmcs_t::field::host_ss_selector, ss.selector);
  return ss;
}

void vcpu_t::host_ss(segment_t<ss_t> ss) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_ss_selector, ss.selector.index * 8);
}

auto vcpu_t::host_tr() const noexcept -> segment_t<tr_t>
{
  segment_t<tr_t> tr;
  vmx::vmread(vmx::vmcs_t::field::host_tr_selector, tr.selector);
  vmx::vmread(vmx::vmcs_t::field::host_tr_base, tr.base_address);
  return tr;
}

void vcpu_t::host_tr(segment_t<tr_t> tr) noexcept
{
  vmx::vmwrite(vmx::vmcs_t::field::host_tr_selector, tr.selector.index * 8);
  vmx::vmwrite(vmx::vmcs_t::field::host_tr_base, tr.base_address);
}

}
