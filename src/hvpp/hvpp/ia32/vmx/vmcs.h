#pragma once
#include "../memory.h"

#include <cstdint>

namespace ia32::vmx {

namespace detail
{
  enum class vmcs_access_type_t : uint16_t
  {
    full    = 0,
    high    = 1,
  };

  enum class vmcs_type_t : uint16_t
  {
    control = 0,
    vmexit  = 1,
    guest   = 2,
    host    = 3
  };

  enum class vmcs_width_t : uint16_t
  {
    _16_bit = 0,
    _64_bit = 1,
    _32_bit = 2,
    natural = 3,
  };

  struct vmcs_component_encoding
  {
    union
    {
      uint16_t flags;

      struct
      {
        uint16_t access_type : 1;
        uint16_t index : 9;
        uint16_t type : 2;
        uint16_t must_be_zero : 1;
        uint16_t width : 2;
      };
    };
  };

  static constexpr int width_to_bits(vmcs_width_t width) noexcept
  {
    return
      width == detail::vmcs_width_t::_16_bit ? 2 :
      width == detail::vmcs_width_t::_64_bit ? 8 :
      width == detail::vmcs_width_t::_32_bit ? 4 :
      width == detail::vmcs_width_t::natural ? 8 : -1;
  }

  static constexpr uint16_t encode(vmcs_access_type_t access_type, vmcs_type_t type, vmcs_width_t width, uint16_t index) noexcept
  {
    //
    // Visual Studio has some problems understanding bitfields in constexpr
    // functions...  let's just use good ol' bit operations.
    //
    // vmcs_componing_encoding result{};
    // result.access_type = static_cast<uint16_t>(access_type);
    // result.type        = static_cast<uint16_t>(type);
    // result.width       = static_cast<uint16_t>(width);
    // result.index       = index;
    //
    // return result.flags;
    //

    return
      (static_cast<uint16_t>(access_type))  |
      (static_cast<uint16_t>(index) << 1)   |
      (static_cast<uint16_t>(type)  << 10)  |
      (static_cast<uint16_t>(width) << 13);
  }

  static constexpr uint16_t encode_full(vmcs_type_t type, vmcs_width_t width, uint16_t index) noexcept
  {
    return encode(vmcs_access_type_t::full, type, width, index);
  }

  static constexpr void decode(uint16_t field, vmcs_access_type_t& access_type, vmcs_type_t& type, vmcs_width_t& width, uint16_t& index) noexcept
  {
    access_type = static_cast<vmcs_access_type_t>( field        &     1);
    type        = static_cast<vmcs_type_t>       ((field >> 10) &     3);
    width       = static_cast<vmcs_width_t>      ((field >> 13) &     3);
    index       = static_cast<uint16_t>          ((field >>  1) & 0x1ff);
  }
}

struct alignas(page_size) vmcs_t
{
  uint32_t revision_id;
  uint32_t abort_indicator;

private:
  uint8_t  data[page_size - 2 * sizeof(uint32_t)];

  using type  = detail::vmcs_type_t;
  using width = detail::vmcs_width_t;

public:
  enum class field
  {
    //
    // [CONTROL]
    //

    //
    // control::16_bit
    //
    ctrl_virtual_processor_identifier                    = detail::encode_full(type::control, width::_16_bit, 0),
    ctrl_posted_interrupt_notification_vector            = detail::encode_full(type::control, width::_16_bit, 1),
    ctrl_eptp_index                                      = detail::encode_full(type::control, width::_16_bit, 2),

    //
    // control::64_bit
    //
    ctrl_io_bitmap_a_address                             = detail::encode_full(type::control, width::_64_bit, 0),
    ctrl_io_bitmap_b_address                             = detail::encode_full(type::control, width::_64_bit, 1),
    ctrl_msr_bitmap_address                              = detail::encode_full(type::control, width::_64_bit, 2),
    ctrl_vmexit_msr_store_address                        = detail::encode_full(type::control, width::_64_bit, 3),
    ctrl_vmexit_msr_load_address                         = detail::encode_full(type::control, width::_64_bit, 4),
    ctrl_vmentry_msr_load_address                        = detail::encode_full(type::control, width::_64_bit, 5),
    ctrl_executive_vmcs_pointer                          = detail::encode_full(type::control, width::_64_bit, 6),
    ctrl_pml_address                                     = detail::encode_full(type::control, width::_64_bit, 7),
    ctrl_tsc_offset                                      = detail::encode_full(type::control, width::_64_bit, 8),
    ctrl_virtual_apic_address                            = detail::encode_full(type::control, width::_64_bit, 9),
    ctrl_apic_access_address                             = detail::encode_full(type::control, width::_64_bit, 10),
    ctrl_posted_interrupt_descriptor_address             = detail::encode_full(type::control, width::_64_bit, 11),
    ctrl_vmfunc_controls                                 = detail::encode_full(type::control, width::_64_bit, 12),
    ctrl_ept_pointer                                     = detail::encode_full(type::control, width::_64_bit, 13),
    ctrl_eoi_exit_bitmap_0                               = detail::encode_full(type::control, width::_64_bit, 14),
    ctrl_eoi_exit_bitmap_1                               = detail::encode_full(type::control, width::_64_bit, 15),
    ctrl_eoi_exit_bitmap_2                               = detail::encode_full(type::control, width::_64_bit, 16),
    ctrl_eoi_exit_bitmap_3                               = detail::encode_full(type::control, width::_64_bit, 17),
    ctrl_ept_pointer_list_address                        = detail::encode_full(type::control, width::_64_bit, 18),
    ctrl_vmread_bitmap_address                           = detail::encode_full(type::control, width::_64_bit, 19),
    ctrl_vmwrite_bitmap_address                          = detail::encode_full(type::control, width::_64_bit, 20),
    ctrl_virtualization_exception_info_address           = detail::encode_full(type::control, width::_64_bit, 21),
    ctrl_xss_exiting_bitmap                              = detail::encode_full(type::control, width::_64_bit, 22),
    ctrl_encls_exiting_bitmap                            = detail::encode_full(type::control, width::_64_bit, 23),
    ctrl_tsc_multiplier                                  = detail::encode_full(type::control, width::_64_bit, 25),

    //
    // control::32_bit
    //
    ctrl_pin_based_vm_execution_controls                 = detail::encode_full(type::control, width::_32_bit, 0),
    ctrl_processor_based_vm_execution_controls           = detail::encode_full(type::control, width::_32_bit, 1),
    ctrl_exception_bitmap                                = detail::encode_full(type::control, width::_32_bit, 2),
    ctrl_pagefault_error_code_mask                       = detail::encode_full(type::control, width::_32_bit, 3),
    ctrl_pagefault_error_code_match                      = detail::encode_full(type::control, width::_32_bit, 4),
    ctrl_cr3_target_count                                = detail::encode_full(type::control, width::_32_bit, 5),
    ctrl_vmexit_controls                                 = detail::encode_full(type::control, width::_32_bit, 6),
    ctrl_vmexit_msr_store_count                          = detail::encode_full(type::control, width::_32_bit, 7),
    ctrl_vmexit_msr_load_count                           = detail::encode_full(type::control, width::_32_bit, 8),
    ctrl_vmentry_controls                                = detail::encode_full(type::control, width::_32_bit, 9),
    ctrl_vmentry_msr_load_count                          = detail::encode_full(type::control, width::_32_bit, 10),
    ctrl_vmentry_interruption_info                       = detail::encode_full(type::control, width::_32_bit, 11),
    ctrl_vmentry_exception_error_code                    = detail::encode_full(type::control, width::_32_bit, 12),
    ctrl_vmentry_instruction_length                      = detail::encode_full(type::control, width::_32_bit, 13),
    ctrl_tpr_threshold                                   = detail::encode_full(type::control, width::_32_bit, 14),
    ctrl_secondary_processor_based_vm_execution_controls = detail::encode_full(type::control, width::_32_bit, 15),
    ctrl_ple_gap                                         = detail::encode_full(type::control, width::_32_bit, 16),
    ctrl_ple_window                                      = detail::encode_full(type::control, width::_32_bit, 17),

    //
    // control::natural
    //
    ctrl_cr0_guest_host_mask                             = detail::encode_full(type::control, width::natural, 0),
    ctrl_cr4_guest_host_mask                             = detail::encode_full(type::control, width::natural, 1),
    ctrl_cr0_read_shadow                                 = detail::encode_full(type::control, width::natural, 2),
    ctrl_cr4_read_shadow                                 = detail::encode_full(type::control, width::natural, 3),
    ctrl_cr3_target_value_0                              = detail::encode_full(type::control, width::natural, 4),
    ctrl_cr3_target_value_1                              = detail::encode_full(type::control, width::natural, 5),
    ctrl_cr3_target_value_2                              = detail::encode_full(type::control, width::natural, 6),
    ctrl_cr3_target_value_3                              = detail::encode_full(type::control, width::natural, 7),

    //
    // [VMEXIT] (read-only)
    //

    //
    // vmexit::16_bit
    //

    //
    // vmexit::64_bit
    //
    vmexit_guest_physical_address                        = detail::encode_full(type::vmexit, width::_64_bit, 0),

    //
    // vmexit::32_bit
    //
    vmexit_instruction_error                             = detail::encode_full(type::vmexit, width::_32_bit, 0),
    vmexit_reason                                        = detail::encode_full(type::vmexit, width::_32_bit, 1),
    vmexit_interruption_info                             = detail::encode_full(type::vmexit, width::_32_bit, 2),
    vmexit_interruption_error_code                       = detail::encode_full(type::vmexit, width::_32_bit, 3),
    vmexit_idt_vectoring_info                            = detail::encode_full(type::vmexit, width::_32_bit, 4),
    vmexit_idt_vectoring_error_code                      = detail::encode_full(type::vmexit, width::_32_bit, 5),
    vmexit_instruction_length                            = detail::encode_full(type::vmexit, width::_32_bit, 6),
    vmexit_instruction_info                              = detail::encode_full(type::vmexit, width::_32_bit, 7),

    //
    // vmexit::natural
    //
    vmexit_qualification                                 = detail::encode_full(type::vmexit, width::natural, 0),
    vmexit_io_rcx                                        = detail::encode_full(type::vmexit, width::natural, 1),
    vmexit_io_rsx                                        = detail::encode_full(type::vmexit, width::natural, 2),
    vmexit_io_rdi                                        = detail::encode_full(type::vmexit, width::natural, 3),
    vmexit_io_rip                                        = detail::encode_full(type::vmexit, width::natural, 4),
    vmexit_guest_linear_address                          = detail::encode_full(type::vmexit, width::natural, 5),

    //
    // [GUEST]
    //

    //
    // guest::16_bit
    //
    guest_es_selector                                    = detail::encode_full(type::guest, width::_16_bit, 0),
    guest_cs_selector                                    = detail::encode_full(type::guest, width::_16_bit, 1),
    guest_ss_selector                                    = detail::encode_full(type::guest, width::_16_bit, 2),
    guest_ds_selector                                    = detail::encode_full(type::guest, width::_16_bit, 3),
    guest_fs_selector                                    = detail::encode_full(type::guest, width::_16_bit, 4),
    guest_gs_selector                                    = detail::encode_full(type::guest, width::_16_bit, 5),
    guest_ldtr_selector                                  = detail::encode_full(type::guest, width::_16_bit, 6),
    guest_tr_selector                                    = detail::encode_full(type::guest, width::_16_bit, 7),
    guest_interrupt_status                               = detail::encode_full(type::guest, width::_16_bit, 8),
    guest_pml_index                                      = detail::encode_full(type::guest, width::_16_bit, 9),

    //
    // guest::64_bit
    //
    guest_vmcs_link_pointer                              = detail::encode_full(type::guest, width::_64_bit, 0),
    guest_debugctl                                       = detail::encode_full(type::guest, width::_64_bit, 1),
    guest_pat                                            = detail::encode_full(type::guest, width::_64_bit, 2),
    guest_efer                                           = detail::encode_full(type::guest, width::_64_bit, 3),
    guest_perf_global_ctrl                               = detail::encode_full(type::guest, width::_64_bit, 4),
    guest_pdpte0                                         = detail::encode_full(type::guest, width::_64_bit, 5),
    guest_pdpte1                                         = detail::encode_full(type::guest, width::_64_bit, 6),
    guest_pdpte2                                         = detail::encode_full(type::guest, width::_64_bit, 7),
    guest_pdpte3                                         = detail::encode_full(type::guest, width::_64_bit, 8),

    //
    // guest::32_bit
    //
    guest_es_limit                                       = detail::encode_full(type::guest, width::_32_bit, 0),
    guest_cs_limit                                       = detail::encode_full(type::guest, width::_32_bit, 1),
    guest_ss_limit                                       = detail::encode_full(type::guest, width::_32_bit, 2),
    guest_ds_limit                                       = detail::encode_full(type::guest, width::_32_bit, 3),
    guest_fs_limit                                       = detail::encode_full(type::guest, width::_32_bit, 4),
    guest_gs_limit                                       = detail::encode_full(type::guest, width::_32_bit, 5),
    guest_ldtr_limit                                     = detail::encode_full(type::guest, width::_32_bit, 6),
    guest_tr_limit                                       = detail::encode_full(type::guest, width::_32_bit, 7),
    guest_gdtr_limit                                     = detail::encode_full(type::guest, width::_32_bit, 8),
    guest_idtr_limit                                     = detail::encode_full(type::guest, width::_32_bit, 9),
    guest_es_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 10),
    guest_cs_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 11),
    guest_ss_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 12),
    guest_ds_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 13),
    guest_fs_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 14),
    guest_gs_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 15),
    guest_ldtr_access_rights                             = detail::encode_full(type::guest, width::_32_bit, 16),
    guest_tr_access_rights                               = detail::encode_full(type::guest, width::_32_bit, 17),
    guest_interruptibility_state                         = detail::encode_full(type::guest, width::_32_bit, 18),
    guest_activity_state                                 = detail::encode_full(type::guest, width::_32_bit, 19),
    guest_smbase                                         = detail::encode_full(type::guest, width::_32_bit, 20),
    guest_sysenter_cs                                    = detail::encode_full(type::guest, width::_32_bit, 21),
    guest_vmx_preemption_timer_value                     = detail::encode_full(type::guest, width::_32_bit, 23),

    //
    // guest::natural
    //
    guest_cr0                                            = detail::encode_full(type::guest, width::natural, 0),
    guest_cr3                                            = detail::encode_full(type::guest, width::natural, 1),
    guest_cr4                                            = detail::encode_full(type::guest, width::natural, 2),
    guest_es_base                                        = detail::encode_full(type::guest, width::natural, 3),
    guest_cs_base                                        = detail::encode_full(type::guest, width::natural, 4),
    guest_ss_base                                        = detail::encode_full(type::guest, width::natural, 5),
    guest_ds_base                                        = detail::encode_full(type::guest, width::natural, 6),
    guest_fs_base                                        = detail::encode_full(type::guest, width::natural, 7),
    guest_gs_base                                        = detail::encode_full(type::guest, width::natural, 8),
    guest_ldtr_base                                      = detail::encode_full(type::guest, width::natural, 9),
    guest_tr_base                                        = detail::encode_full(type::guest, width::natural, 10),
    guest_gdtr_base                                      = detail::encode_full(type::guest, width::natural, 11),
    guest_idtr_base                                      = detail::encode_full(type::guest, width::natural, 12),
    guest_dr7                                            = detail::encode_full(type::guest, width::natural, 13),
    guest_rsp                                            = detail::encode_full(type::guest, width::natural, 14),
    guest_rip                                            = detail::encode_full(type::guest, width::natural, 15),
    guest_rflags                                         = detail::encode_full(type::guest, width::natural, 16),
    guest_pending_debug_exceptions                       = detail::encode_full(type::guest, width::natural, 17),
    guest_sysenter_esp                                   = detail::encode_full(type::guest, width::natural, 18),
    guest_sysenter_eip                                   = detail::encode_full(type::guest, width::natural, 19),

    //
    // [HOST]
    //

    //
    // host::16_bit
    //
    host_es_selector                                     = detail::encode_full(type::host, width::_16_bit, 0),
    host_cs_selector                                     = detail::encode_full(type::host, width::_16_bit, 1),
    host_ss_selector                                     = detail::encode_full(type::host, width::_16_bit, 2),
    host_ds_selector                                     = detail::encode_full(type::host, width::_16_bit, 3),
    host_fs_selector                                     = detail::encode_full(type::host, width::_16_bit, 4),
    host_gs_selector                                     = detail::encode_full(type::host, width::_16_bit, 5),
    host_tr_selector                                     = detail::encode_full(type::host, width::_16_bit, 6),

    //
    // host::64_bit
    //
    host_pat                                             = detail::encode_full(type::host, width::_64_bit, 0),
    host_efer                                            = detail::encode_full(type::host, width::_64_bit, 1),
    host_perf_global_ctrl                                = detail::encode_full(type::host, width::_64_bit, 2),

    //
    // host::32_bit
    //
    host_sysenter_cs                                     = detail::encode_full(type::host, width::_32_bit, 0),

    //
    // host::natural
    //
    host_cr0                                             = detail::encode_full(type::host, width::natural, 0),
    host_cr3                                             = detail::encode_full(type::host, width::natural, 1),
    host_cr4                                             = detail::encode_full(type::host, width::natural, 2),
    host_fs_base                                         = detail::encode_full(type::host, width::natural, 3),
    host_gs_base                                         = detail::encode_full(type::host, width::natural, 4),
    host_tr_base                                         = detail::encode_full(type::host, width::natural, 5),
    host_gdtr_base                                       = detail::encode_full(type::host, width::natural, 6),
    host_idtr_base                                       = detail::encode_full(type::host, width::natural, 7),
    host_sysenter_esp                                    = detail::encode_full(type::host, width::natural, 8),
    host_sysenter_eip                                    = detail::encode_full(type::host, width::natural, 9),
    host_rsp                                             = detail::encode_full(type::host, width::natural, 10),
    host_rip                                             = detail::encode_full(type::host, width::natural, 11),
  };
};

inline vmcs_t::field operator+(vmcs_t::field vmcs_field, int index) noexcept
{ return static_cast<vmcs_t::field>(static_cast<int>(vmcs_field) + index); }

inline vmcs_t::field operator-(vmcs_t::field vmcs_field, int index) noexcept
{ return static_cast<vmcs_t::field>(static_cast<int>(vmcs_field) - index); }

inline vmcs_t::field& operator+=(vmcs_t::field& vmcs_field, int index) noexcept
{ reinterpret_cast<int&>(vmcs_field) += index; return vmcs_field; }

inline vmcs_t::field& operator-=(vmcs_t::field& vmcs_field, int index) noexcept
{ reinterpret_cast<int&>(vmcs_field) -= index; return vmcs_field; }

static_assert(sizeof(vmcs_t) == page_size);

constexpr inline const char* to_string(vmcs_t::field value) noexcept
{
  switch (value)
  {
    case vmcs_t::field::ctrl_virtual_processor_identifier: return "ctrl_virtual_processor_identifier";
    case vmcs_t::field::ctrl_posted_interrupt_notification_vector: return "ctrl_posted_interrupt_notification_vector";
    case vmcs_t::field::ctrl_eptp_index: return "ctrl_eptp_index";
    case vmcs_t::field::ctrl_io_bitmap_a_address: return "ctrl_io_bitmap_a_address";
    case vmcs_t::field::ctrl_io_bitmap_b_address: return "ctrl_io_bitmap_b_address";
    case vmcs_t::field::ctrl_msr_bitmap_address: return "ctrl_msr_bitmap_address";
    case vmcs_t::field::ctrl_vmexit_msr_store_address: return "ctrl_vmexit_msr_store_address";
    case vmcs_t::field::ctrl_vmexit_msr_load_address: return "ctrl_vmexit_msr_load_address";
    case vmcs_t::field::ctrl_vmentry_msr_load_address: return "ctrl_vmentry_msr_load_address";
    case vmcs_t::field::ctrl_executive_vmcs_pointer: return "ctrl_executive_vmcs_pointer";
    case vmcs_t::field::ctrl_pml_address: return "ctrl_pml_address";
    case vmcs_t::field::ctrl_tsc_offset: return "ctrl_tsc_offset";
    case vmcs_t::field::ctrl_virtual_apic_address: return "ctrl_virtual_apic_address";
    case vmcs_t::field::ctrl_apic_access_address: return "ctrl_apic_access_address";
    case vmcs_t::field::ctrl_posted_interrupt_descriptor_address: return "ctrl_posted_interrupt_descriptor_address";
    case vmcs_t::field::ctrl_vmfunc_controls: return "ctrl_vmfunc_controls";
    case vmcs_t::field::ctrl_ept_pointer: return "ctrl_ept_pointer";
    case vmcs_t::field::ctrl_eoi_exit_bitmap_0: return "ctrl_eoi_exit_bitmap_0";
    case vmcs_t::field::ctrl_eoi_exit_bitmap_1: return "ctrl_eoi_exit_bitmap_1";
    case vmcs_t::field::ctrl_eoi_exit_bitmap_2: return "ctrl_eoi_exit_bitmap_2";
    case vmcs_t::field::ctrl_eoi_exit_bitmap_3: return "ctrl_eoi_exit_bitmap_3";
    case vmcs_t::field::ctrl_ept_pointer_list_address: return "ctrl_ept_pointer_list_address";
    case vmcs_t::field::ctrl_vmread_bitmap_address: return "ctrl_vmread_bitmap_address";
    case vmcs_t::field::ctrl_vmwrite_bitmap_address: return "ctrl_vmwrite_bitmap_address";
    case vmcs_t::field::ctrl_virtualization_exception_info_address: return "ctrl_virtualization_exception_info_address";
    case vmcs_t::field::ctrl_xss_exiting_bitmap: return "ctrl_xss_exiting_bitmap";
    case vmcs_t::field::ctrl_encls_exiting_bitmap: return "ctrl_encls_exiting_bitmap";
    case vmcs_t::field::ctrl_tsc_multiplier: return "ctrl_tsc_multiplier";
    case vmcs_t::field::ctrl_pin_based_vm_execution_controls: return "ctrl_pin_based_vm_execution_controls";
    case vmcs_t::field::ctrl_processor_based_vm_execution_controls: return "ctrl_processor_based_vm_execution_controls";
    case vmcs_t::field::ctrl_exception_bitmap: return "ctrl_exception_bitmap";
    case vmcs_t::field::ctrl_pagefault_error_code_mask: return "ctrl_pagefault_error_code_mask";
    case vmcs_t::field::ctrl_pagefault_error_code_match: return "ctrl_pagefault_error_code_match";
    case vmcs_t::field::ctrl_cr3_target_count: return "ctrl_cr3_target_count";
    case vmcs_t::field::ctrl_vmexit_controls: return "ctrl_vmexit_controls";
    case vmcs_t::field::ctrl_vmexit_msr_store_count: return "ctrl_vmexit_msr_store_count";
    case vmcs_t::field::ctrl_vmexit_msr_load_count: return "ctrl_vmexit_msr_load_count";
    case vmcs_t::field::ctrl_vmentry_controls: return "ctrl_vmentry_controls";
    case vmcs_t::field::ctrl_vmentry_msr_load_count: return "ctrl_vmentry_msr_load_count";
    case vmcs_t::field::ctrl_vmentry_interruption_info: return "ctrl_vmentry_interruption_info";
    case vmcs_t::field::ctrl_vmentry_exception_error_code: return "ctrl_vmentry_exception_error_code";
    case vmcs_t::field::ctrl_vmentry_instruction_length: return "ctrl_vmentry_instruction_length";
    case vmcs_t::field::ctrl_tpr_threshold: return "ctrl_tpr_threshold";
    case vmcs_t::field::ctrl_secondary_processor_based_vm_execution_controls: return "ctrl_secondary_processor_based_vm_execution_controls";
    case vmcs_t::field::ctrl_ple_gap: return "ctrl_ple_gap";
    case vmcs_t::field::ctrl_ple_window: return "ctrl_ple_window";
    case vmcs_t::field::ctrl_cr0_guest_host_mask: return "ctrl_cr0_guest_host_mask";
    case vmcs_t::field::ctrl_cr4_guest_host_mask: return "ctrl_cr4_guest_host_mask";
    case vmcs_t::field::ctrl_cr0_read_shadow: return "ctrl_cr0_read_shadow";
    case vmcs_t::field::ctrl_cr4_read_shadow: return "ctrl_cr4_read_shadow";
    case vmcs_t::field::ctrl_cr3_target_value_0: return "ctrl_cr3_target_value_0";
    case vmcs_t::field::ctrl_cr3_target_value_1: return "ctrl_cr3_target_value_1";
    case vmcs_t::field::ctrl_cr3_target_value_2: return "ctrl_cr3_target_value_2";
    case vmcs_t::field::ctrl_cr3_target_value_3: return "ctrl_cr3_target_value_3";
    case vmcs_t::field::vmexit_guest_physical_address: return "vmexit_guest_physical_address";
    case vmcs_t::field::vmexit_instruction_error: return "vmexit_instruction_error";
    case vmcs_t::field::vmexit_reason: return "vmexit_reason";
    case vmcs_t::field::vmexit_interruption_info: return "vmexit_interruption_info";
    case vmcs_t::field::vmexit_interruption_error_code: return "vmexit_interruption_error_code";
    case vmcs_t::field::vmexit_idt_vectoring_info: return "vmexit_idt_vectoring_info";
    case vmcs_t::field::vmexit_idt_vectoring_error_code: return "vmexit_idt_vectoring_error_code";
    case vmcs_t::field::vmexit_instruction_length: return "vmexit_instruction_length";
    case vmcs_t::field::vmexit_instruction_info: return "vmexit_instruction_info";
    case vmcs_t::field::vmexit_qualification: return "vmexit_qualification";
    case vmcs_t::field::vmexit_io_rcx: return "vmexit_io_rcx";
    case vmcs_t::field::vmexit_io_rsx: return "vmexit_io_rsx";
    case vmcs_t::field::vmexit_io_rdi: return "vmexit_io_rdi";
    case vmcs_t::field::vmexit_io_rip: return "vmexit_io_rip";
    case vmcs_t::field::vmexit_guest_linear_address: return "vmexit_guest_linear_address";
    case vmcs_t::field::guest_es_selector: return "guest_es_selector";
    case vmcs_t::field::guest_cs_selector: return "guest_cs_selector";
    case vmcs_t::field::guest_ss_selector: return "guest_ss_selector";
    case vmcs_t::field::guest_ds_selector: return "guest_ds_selector";
    case vmcs_t::field::guest_fs_selector: return "guest_fs_selector";
    case vmcs_t::field::guest_gs_selector: return "guest_gs_selector";
    case vmcs_t::field::guest_ldtr_selector: return "guest_ldtr_selector";
    case vmcs_t::field::guest_tr_selector: return "guest_tr_selector";
    case vmcs_t::field::guest_interrupt_status: return "guest_interrupt_status";
    case vmcs_t::field::guest_pml_index: return "guest_pml_index";
    case vmcs_t::field::guest_vmcs_link_pointer: return "guest_vmcs_link_pointer";
    case vmcs_t::field::guest_debugctl: return "guest_debugctl";
    case vmcs_t::field::guest_pat: return "guest_pat";
    case vmcs_t::field::guest_efer: return "guest_efer";
    case vmcs_t::field::guest_perf_global_ctrl: return "guest_perf_global_ctrl";
    case vmcs_t::field::guest_pdpte0: return "guest_pdpte0";
    case vmcs_t::field::guest_pdpte1: return "guest_pdpte1";
    case vmcs_t::field::guest_pdpte2: return "guest_pdpte2";
    case vmcs_t::field::guest_pdpte3: return "guest_pdpte3";
    case vmcs_t::field::guest_es_limit: return "guest_es_limit";
    case vmcs_t::field::guest_cs_limit: return "guest_cs_limit";
    case vmcs_t::field::guest_ss_limit: return "guest_ss_limit";
    case vmcs_t::field::guest_ds_limit: return "guest_ds_limit";
    case vmcs_t::field::guest_fs_limit: return "guest_fs_limit";
    case vmcs_t::field::guest_gs_limit: return "guest_gs_limit";
    case vmcs_t::field::guest_ldtr_limit: return "guest_ldtr_limit";
    case vmcs_t::field::guest_tr_limit: return "guest_tr_limit";
    case vmcs_t::field::guest_gdtr_limit: return "guest_gdtr_limit";
    case vmcs_t::field::guest_idtr_limit: return "guest_idtr_limit";
    case vmcs_t::field::guest_es_access_rights: return "guest_es_access_rights";
    case vmcs_t::field::guest_cs_access_rights: return "guest_cs_access_rights";
    case vmcs_t::field::guest_ss_access_rights: return "guest_ss_access_rights";
    case vmcs_t::field::guest_ds_access_rights: return "guest_ds_access_rights";
    case vmcs_t::field::guest_fs_access_rights: return "guest_fs_access_rights";
    case vmcs_t::field::guest_gs_access_rights: return "guest_gs_access_rights";
    case vmcs_t::field::guest_ldtr_access_rights: return "guest_ldtr_access_rights";
    case vmcs_t::field::guest_tr_access_rights: return "guest_tr_access_rights";
    case vmcs_t::field::guest_interruptibility_state: return "guest_interruptibility_state";
    case vmcs_t::field::guest_activity_state: return "guest_activity_state";
    case vmcs_t::field::guest_smbase: return "guest_smbase";
    case vmcs_t::field::guest_sysenter_cs: return "guest_sysenter_cs";
    case vmcs_t::field::guest_vmx_preemption_timer_value: return "guest_vmx_preemption_timer_value";
    case vmcs_t::field::guest_cr0: return "guest_cr0";
    case vmcs_t::field::guest_cr3: return "guest_cr3";
    case vmcs_t::field::guest_cr4: return "guest_cr4";
    case vmcs_t::field::guest_es_base: return "guest_es_base";
    case vmcs_t::field::guest_cs_base: return "guest_cs_base";
    case vmcs_t::field::guest_ss_base: return "guest_ss_base";
    case vmcs_t::field::guest_ds_base: return "guest_ds_base";
    case vmcs_t::field::guest_fs_base: return "guest_fs_base";
    case vmcs_t::field::guest_gs_base: return "guest_gs_base";
    case vmcs_t::field::guest_ldtr_base: return "guest_ldtr_base";
    case vmcs_t::field::guest_tr_base: return "guest_tr_base";
    case vmcs_t::field::guest_gdtr_base: return "guest_gdtr_base";
    case vmcs_t::field::guest_idtr_base: return "guest_idtr_base";
    case vmcs_t::field::guest_dr7: return "guest_dr7";
    case vmcs_t::field::guest_rsp: return "guest_rsp";
    case vmcs_t::field::guest_rip: return "guest_rip";
    case vmcs_t::field::guest_rflags: return "guest_rflags";
    case vmcs_t::field::guest_pending_debug_exceptions: return "guest_pending_debug_exceptions";
    case vmcs_t::field::guest_sysenter_esp: return "guest_sysenter_esp";
    case vmcs_t::field::guest_sysenter_eip: return "guest_sysenter_eip";
    case vmcs_t::field::host_es_selector: return "host_es_selector";
    case vmcs_t::field::host_cs_selector: return "host_cs_selector";
    case vmcs_t::field::host_ss_selector: return "host_ss_selector";
    case vmcs_t::field::host_ds_selector: return "host_ds_selector";
    case vmcs_t::field::host_fs_selector: return "host_fs_selector";
    case vmcs_t::field::host_gs_selector: return "host_gs_selector";
    case vmcs_t::field::host_tr_selector: return "host_tr_selector";
    case vmcs_t::field::host_pat: return "host_pat";
    case vmcs_t::field::host_efer: return "host_efer";
    case vmcs_t::field::host_perf_global_ctrl: return "host_perf_global_ctrl";
    case vmcs_t::field::host_sysenter_cs: return "host_sysenter_cs";
    case vmcs_t::field::host_cr0: return "host_cr0";
    case vmcs_t::field::host_cr3: return "host_cr3";
    case vmcs_t::field::host_cr4: return "host_cr4";
    case vmcs_t::field::host_fs_base: return "host_fs_base";
    case vmcs_t::field::host_gs_base: return "host_gs_base";
    case vmcs_t::field::host_tr_base: return "host_tr_base";
    case vmcs_t::field::host_gdtr_base: return "host_gdtr_base";
    case vmcs_t::field::host_idtr_base: return "host_idtr_base";
    case vmcs_t::field::host_sysenter_esp: return "host_sysenter_esp";
    case vmcs_t::field::host_sysenter_eip: return "host_sysenter_eip";
    case vmcs_t::field::host_rsp: return "host_rsp";
    case vmcs_t::field::host_rip: return "host_rip";
  }

  return "";
}

}
