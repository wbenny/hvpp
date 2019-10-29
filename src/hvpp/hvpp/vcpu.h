#pragma once
#include "ept.h"
#include "interrupt.h"

#include "ia32/arch.h"

#include "lib/deque.h"
#include "lib/error.h"
#include "lib/spinlock.h"

#include "lib/mm/memory_mapper.h"
#include "lib/mm/memory_translator.h"

#include <cstdint>

namespace hvpp {

using namespace ia32;

class vmexit_handler;

class vcpu_t final
{
  public:
    vcpu_t(vmexit_handler& handler) noexcept;
    ~vcpu_t() noexcept;

    auto start() noexcept -> error_code_t;
    void stop() noexcept;

    auto vmx_enter() noexcept -> error_code_t;
    void vmx_leave() noexcept;

    void ept_enable() noexcept;
    void ept_disable() noexcept;
    bool ept_is_enabled() const noexcept;

    auto ept() noexcept -> ept_t&;
    void ept(ept_t& new_ept) noexcept;

    auto context() noexcept -> context_t&;
    void suppress_rip_adjust() noexcept;

    auto user_data() noexcept -> void*;
    void user_data(void* data) noexcept;

    //
    // Guest helper methods.
    //

    [[noreturn]]
    void guest_resume() noexcept;

    auto guest_memory_mapper() noexcept -> mm::memory_mapper&;
    auto guest_memory_translator() noexcept -> mm::memory_translator&;

    auto guest_va_to_pa(va_t guest_va) noexcept -> pa_t;
    auto guest_read_memory(va_t guest_va, void* buffer, size_t size, bool ignore_errors = false) noexcept -> va_t;
    auto guest_write_memory(va_t guest_va, const void* buffer, size_t size, bool ignore_errors = false) noexcept -> va_t;

    auto tsc_entry() const noexcept -> uint64_t;
    auto tsc_delta_previous() const noexcept -> uint64_t;
    auto tsc_delta_sum() const noexcept -> uint64_t;

    //
    // Stacked lock guard.
    //

    struct stacked_lock_guard_t
    {
      stacked_lock_guard_t(vcpu_t& vp, spinlock& lock) noexcept;
      ~stacked_lock_guard_t() noexcept;

      stacked_lock_guard_t(const stacked_lock_guard_t& other) noexcept = delete;
      stacked_lock_guard_t(stacked_lock_guard_t&& other) noexcept = delete;
      stacked_lock_guard_t& operator=(const stacked_lock_guard_t& other) noexcept = delete;
      stacked_lock_guard_t& operator=(stacked_lock_guard_t&& other) noexcept = delete;

    private:
      vcpu_t& vp_;
    };

    auto stacked_lock_guard(spinlock& lock) noexcept -> stacked_lock_guard_t;
    void stacked_lock_guard_push(spinlock& lock) noexcept;
    void stacked_lock_guard_pop() noexcept;

    //
    // VMCS manipulation. Implementation is in vcpu.inl.
    //

  public:
    //
    // Pending interrupt queue (FIFO).
    // Make storage for up-to 64 pending interrupts.
    // In practice I haven't seen more than 2 pending interrupts.
    //
    using interrupt_queue_t = fixed_dequeue<interrupt_t, 64>;

    enum interrupt_queue_type
    {
      interrupt_queue_external, // vmx::interrupt_type::external (0)
      interrupt_queue_nmi,      // vmx::interrupt_type::nmi (2)

      interrupt_queue_max
    };

    auto interrupt_info() const noexcept -> interrupt_t;
    auto idt_vectoring_info() const noexcept -> interrupt_t;

    bool interrupt_inject(interrupt_t interrupt, bool front = false) noexcept;
    void interrupt_inject_force(interrupt_t interrupt) noexcept;
    void interrupt_inject_pending(interrupt_queue_type queue_type) noexcept;
    bool interrupt_is_pending(interrupt_queue_type queue_type) const noexcept;

    auto exit_instruction_info_guest_va() const noexcept -> void*;

    //
    // Control state
    //

  public:
    //
    // (public read-only fields)
    //

    auto vcpu_id() const noexcept -> uint16_t;
    auto ept_pointer() const noexcept -> ept_ptr_t;
    auto vmcs_link_pointer() const noexcept -> pa_t;     // technically, this is guest state

  private:
    void vcpu_id(uint16_t virtual_processor_identifier) noexcept;
    void ept_pointer(ept_ptr_t ept_pointer) noexcept;
    void vmcs_link_pointer(pa_t link_pointer) noexcept;

  public:
    auto pin_based_controls() const noexcept -> msr::vmx_pinbased_ctls_t;
    void pin_based_controls(msr::vmx_pinbased_ctls_t controls) noexcept;

    auto processor_based_controls() const noexcept -> msr::vmx_procbased_ctls_t;
    void processor_based_controls(msr::vmx_procbased_ctls_t controls) noexcept;

    auto processor_based_controls2() const noexcept -> msr::vmx_procbased_ctls2_t;
    void processor_based_controls2(msr::vmx_procbased_ctls2_t controls) noexcept;

    auto vm_entry_controls() const noexcept -> msr::vmx_entry_ctls_t;
    void vm_entry_controls(msr::vmx_entry_ctls_t controls) noexcept;

    auto vm_exit_controls() const noexcept -> msr::vmx_exit_ctls_t;
    void vm_exit_controls(msr::vmx_exit_ctls_t controls) noexcept;

    auto exception_bitmap() const noexcept -> vmx::exception_bitmap_t;
    void exception_bitmap(vmx::exception_bitmap_t exception_bitmap) noexcept;
    auto msr_bitmap() const noexcept -> const vmx::msr_bitmap_t&;
    void msr_bitmap(const vmx::msr_bitmap_t& msr_bitmap) noexcept;
    auto io_bitmap() const noexcept -> const vmx::io_bitmap_t&;
    void io_bitmap(const vmx::io_bitmap_t& io_bitmap) noexcept;

    auto pagefault_error_code_mask() const noexcept -> pagefault_error_code_t;
    void pagefault_error_code_mask(pagefault_error_code_t mask) noexcept;
    auto pagefault_error_code_match() const noexcept -> pagefault_error_code_t;
    void pagefault_error_code_match(pagefault_error_code_t match) noexcept;

    //
    // Control entry state
    //

    auto cr0_guest_host_mask() const noexcept -> cr0_t;
    void cr0_guest_host_mask(cr0_t cr0) noexcept;
    auto cr0_shadow() const noexcept -> cr0_t;
    void cr0_shadow(cr0_t cr0) noexcept;
    auto cr4_guest_host_mask() const noexcept -> cr4_t;
    void cr4_guest_host_mask(cr4_t cr4) noexcept;
    auto cr4_shadow() const noexcept -> cr4_t;
    void cr4_shadow(cr4_t cr4) noexcept;

    auto entry_instruction_length() const noexcept -> uint32_t;
    void entry_instruction_length(uint32_t instruction_length) noexcept;

    auto entry_interruption_info() const noexcept -> vmx::interrupt_info_t;
    void entry_interruption_info(vmx::interrupt_info_t info) noexcept;

    auto entry_interruption_error_code() const noexcept -> exception_error_code_t;
    void entry_interruption_error_code(exception_error_code_t error_code) noexcept;

    //
    // Exit state
    //

    auto exit_instruction_error() const noexcept -> vmx::instruction_error;
    auto exit_instruction_info() const noexcept -> vmx::instruction_info_t;
    auto exit_instruction_length() const noexcept -> uint32_t;

    auto exit_interruption_info() const noexcept -> vmx::interrupt_info_t;
    auto exit_interruption_error_code() const noexcept -> exception_error_code_t;

    auto exit_idt_vectoring_info() const noexcept -> vmx::interrupt_info_t;
    auto exit_idt_vectoring_error_code() const noexcept -> exception_error_code_t;

    auto exit_reason() const noexcept -> vmx::exit_reason;
    auto exit_qualification() const noexcept -> vmx::exit_qualification_t;
    auto exit_guest_physical_address() const noexcept -> pa_t;
    auto exit_guest_linear_address() const noexcept -> va_t;

    //
    // Guest state
    //

    auto guest_cr0() const noexcept -> cr0_t;
    void guest_cr0(cr0_t cr0) noexcept;
    auto guest_cr3() const noexcept -> cr3_t;
    void guest_cr3(cr3_t cr3) noexcept;
    auto guest_cr4() const noexcept -> cr4_t;
    void guest_cr4(cr4_t cr4) noexcept;

    auto guest_dr7() const noexcept -> dr7_t;
    void guest_dr7(dr7_t dr7) noexcept;
    auto guest_debugctl() const noexcept -> msr::debugctl_t;
    void guest_debugctl(msr::debugctl_t debugctl) noexcept;

    auto guest_rsp() const noexcept -> uint64_t;
    void guest_rsp(uint64_t rsp) noexcept;
    auto guest_rip() const noexcept -> uint64_t;
    void guest_rip(uint64_t rip) noexcept;
    auto guest_rflags() const noexcept -> rflags_t;
    void guest_rflags(rflags_t rflags) noexcept;

    auto guest_gdtr() const noexcept -> gdtr_t;
    void guest_gdtr(gdtr_t gdtr) noexcept;
    auto guest_idtr() const noexcept -> idtr_t;
    void guest_idtr(idtr_t idtr) noexcept;

    auto guest_cs() const noexcept -> segment_t<cs_t>;
    void guest_cs(segment_t<cs_t> cs) noexcept;
    auto guest_ds() const noexcept -> segment_t<ds_t>;
    void guest_ds(segment_t<ds_t> ds) noexcept;
    auto guest_es() const noexcept -> segment_t<es_t>;
    void guest_es(segment_t<es_t> es) noexcept;
    auto guest_fs() const noexcept -> segment_t<fs_t>;
    void guest_fs(segment_t<fs_t> fs) noexcept;
    auto guest_gs() const noexcept -> segment_t<gs_t>;
    void guest_gs(segment_t<gs_t> gs) noexcept;
    auto guest_ss() const noexcept -> segment_t<ss_t>;
    void guest_ss(segment_t<ss_t> ss) noexcept;
    auto guest_tr() const noexcept -> segment_t<tr_t>;
    void guest_tr(segment_t<tr_t> tr) noexcept;
    auto guest_ldtr() const noexcept -> segment_t<ldtr_t>;
    void guest_ldtr(segment_t<ldtr_t> ldtr) noexcept;

    auto guest_segment_base_address(int index) const noexcept -> void*;
    void guest_segment_base_address(int index, void* base_address) noexcept;
    auto guest_segment_limit(int index) const noexcept -> uint32_t;
    void guest_segment_limit(int index, uint32_t limit) noexcept;
    auto guest_segment_access(int index) const noexcept -> segment_access_vmx_t;
    void guest_segment_access(int index, segment_access_vmx_t access_rights) noexcept;
    auto guest_segment_selector(int index) const noexcept -> segment_selector_t;
    void guest_segment_selector(int index, segment_selector_t selector) noexcept;

    auto guest_segment(int index) const noexcept -> segment_t<>;
    void guest_segment(int index, segment_t<> seg) noexcept;

    auto guest_interruptibility_state() const noexcept -> vmx::interruptibility_state_t;
    void guest_interruptibility_state(vmx::interruptibility_state_t interruptibility_state) noexcept;

  private:
    //
    // Host state
    //

    auto host_cr0() const noexcept -> cr0_t;
    void host_cr0(cr0_t cr0) noexcept;
    auto host_cr3() const noexcept -> cr3_t;
    void host_cr3(cr3_t cr3) noexcept;
    auto host_cr4() const noexcept -> cr4_t;
    void host_cr4(cr4_t cr4) noexcept;

    auto host_rsp() const noexcept -> uint64_t;
    void host_rsp(uint64_t rsp) noexcept;
    auto host_rip() const noexcept -> uint64_t;
    void host_rip(uint64_t rip) noexcept;

    auto host_gdtr() const noexcept -> gdtr_t;
    void host_gdtr(gdtr_t gdtr) noexcept;
    auto host_idtr() const noexcept -> idtr_t;
    void host_idtr(idtr_t idtr) noexcept;

    auto host_cs() const noexcept -> segment_t<cs_t>;
    void host_cs(segment_t<cs_t> cs) noexcept;
    auto host_ds() const noexcept -> segment_t<ds_t>;
    void host_ds(segment_t<ds_t> ds) noexcept;
    auto host_es() const noexcept -> segment_t<es_t>;
    void host_es(segment_t<es_t> es) noexcept;
    auto host_fs() const noexcept -> segment_t<fs_t>;
    void host_fs(segment_t<fs_t> fs) noexcept;
    auto host_gs() const noexcept -> segment_t<gs_t>;
    void host_gs(segment_t<gs_t> gs) noexcept;
    auto host_ss() const noexcept -> segment_t<ss_t>;
    void host_ss(segment_t<ss_t> ss) noexcept;
    auto host_tr() const noexcept -> segment_t<tr_t>;
    void host_tr(segment_t<tr_t> tr) noexcept;

    //
    // ldtr does not exist in VMX-root mode.
    //
    // auto host_ldtr() const noexcept -> segment_t<ldtr_t>;
    // void host_ldtr(segment_t<ldtr_t> ldtr)  noexcept;
    //

  private:
    auto handle_common_error(error_code_t err) noexcept -> error_code_t;
    auto handle_vmx_enter_error(error_code_t err) noexcept -> error_code_t;
    auto handle_vmx_launch_error() noexcept -> error_code_t;

    auto load_vmxon() noexcept -> error_code_t;
    auto load_vmcs() noexcept -> error_code_t;

    auto setup_host() noexcept -> error_code_t;
    auto setup_guest() noexcept -> error_code_t;

    void entry_host() noexcept;
    void entry_guest() noexcept;

    static void entry_host_() noexcept;
    static void entry_guest_() noexcept;

    enum class state
    {
      //
      // VCPU is unitialized.
      //
      off,

      //
      // VCPU is in VMX-root mode; host & guest VMCS is being initialized.
      //
      initializing,

      //
      // VCPU successfully performed its initial VMENTRY.
      //
      launching,

      //
      // VCPU is running.
      //
      running,

      //
      // VCPU is terminating; vcpu::destroy has been called.
      //
      terminating,

      //
      // VCPU is terminated, VMX-root mode has been left.
      //
      terminated,
    };

    //
    // Definition of the stack structure.
    // See vcpu.asm for more details.
    //

    struct stack_t
    {
      static constexpr auto size = 0x8000;

      struct machine_frame_t
      {
        uint64_t rip;
        uint64_t cs;
        uint64_t eflags;
        uint64_t rsp;
        uint64_t ss;
      };

      struct shadow_space_t
      {
        uint64_t dummy[4];
      };

      union
      {
        uint8_t data[size];

        struct
        {
          uint8_t         dummy[size
                                - sizeof(shadow_space_t)
                                - sizeof(machine_frame_t)
                                - sizeof(uint64_t)];
          shadow_space_t  shadow_space;
          machine_frame_t machine_frame;
          uint64_t        unused;
        };
      };
    };

    using spinlock_queue_t = fixed_dequeue<spinlock*, 32>;

    static_assert(sizeof(stack_t) == stack_t::size);
    static_assert(sizeof(stack_t::shadow_space_t) == 32);

    //
    // If you reorder following three members (stack, exit context
    // and launch context), you have to edit offsets in vcpu.asm.
    //
    stack_t               stack_;

    union
    {
      //
      // As these two contexts are never used at the same time,
      // they can share the memory.
      //
      context_t           context_;
      context_t           launch_context_;
    };

    //
    // Various VMX structures.
    // Keep in mind they have "alignas(PAGE_SIZE)" specifier.
    //
    vmx::vmcs_t           vmxon_;
    vmx::vmcs_t           vmcs_;
    vmx::msr_bitmap_t     msr_bitmap_;
    vmx::io_bitmap_t      io_bitmap_;

    //
    // FXSAVE area - to keep SSE registers sane between VM-exits.
    //
    fxsave_area_t         fxsave_area_;

    vmexit_handler&       handler_;
    state                 state_;

    ept_t*                ept_;

    //
    // Guest-resume support.
    //
    context_t             resume_context_;
    spinlock_queue_t      spinlock_queue_;

    //
    // Memory translation support.
    //
    mm::memory_mapper     mapper_;
    mm::memory_translator translator_;

    //
    // Timestamp-counter.
    //
    uint64_t              tsc_entry_;
    uint64_t              tsc_delta_previous_;
    uint64_t              tsc_delta_sum_;

    //
    // Pending interrupt queue (FIFO).
    //
    interrupt_queue_t     pending_interrupt_queue_[interrupt_queue_max];

    void*                 user_data_;

    bool                  suppress_rip_adjust_;
};

}
