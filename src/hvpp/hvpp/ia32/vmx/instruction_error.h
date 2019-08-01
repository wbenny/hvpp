#pragma once
#include <cstdint>

namespace ia32::vmx {

enum instruction_error : uint32_t
{
  no_error                                =  0,
  vmcall_in_vmx_root_operation            =  1,
  vmclear_invalid_physical_address        =  2,
  vmclear_invalid_vmxon_pointer           =  3,
  vmlauch_non_clear_vmcs                  =  4,
  vmresume_non_launched_vmcs              =  5,
  vmresume_after_vmxoff                   =  6,
  vmentry_invalid_control_fields          =  7,
  vmentry_invalid_host_state              =  8,
  vmptrld_invalid_physical_address        =  9,
  vmptrld_vmxon_pointer                   = 10,
  vmptrld_incorrect_vmcs_revision_id      = 11,
  vmread_vmwrite_invalid_component        = 12,
  vmwrite_readonly_component              = 13,
  reserved_1                              = 14,
  vmxon_in_vmx_root_op                    = 15,
  vmentry_invalid_vmcs_executive_pointer  = 16,
  vmentry_non_launched_executive_vmcs     = 17,
  vmentry_executive_vmcs_ptr              = 18,
  vmcall_non_clear_vmcs                   = 19,
  vmcall_invalid_vmexit_fields            = 20,
  reserved_2                              = 21,
  vmcall_invalid_mseg_revision_id         = 22,
  vmxoff_dual_monitor                     = 23,
  vmcall_invalid_smm_monitor              = 24,
  vmentry_invalid_vm_execution_control    = 25,
  vmentry_mov_ss                          = 26,
  reserved_3                              = 27,
  invept_invvpid_invalid_operand          = 28,
};

constexpr inline const char* to_string(instruction_error value) noexcept
{
  switch (value)
  {
    case instruction_error::no_error: return "no_error";
    case instruction_error::vmcall_in_vmx_root_operation: return "vmcall_in_vmx_root_operation";
    case instruction_error::vmclear_invalid_physical_address: return "vmclear_invalid_physical_address";
    case instruction_error::vmclear_invalid_vmxon_pointer: return "vmclear_invalid_vmxon_pointer";
    case instruction_error::vmlauch_non_clear_vmcs: return "vmlauch_non_clear_vmcs";
    case instruction_error::vmresume_non_launched_vmcs: return "vmresume_non_launched_vmcs";
    case instruction_error::vmresume_after_vmxoff: return "vmresume_after_vmxoff";
    case instruction_error::vmentry_invalid_control_fields: return "vmentry_invalid_control_fields";
    case instruction_error::vmentry_invalid_host_state: return "vmentry_invalid_host_state";
    case instruction_error::vmptrld_invalid_physical_address: return "vmptrld_invalid_physical_address";
    case instruction_error::vmptrld_vmxon_pointer: return "vmptrld_vmxon_pointer";
    case instruction_error::vmptrld_incorrect_vmcs_revision_id: return "vmptrld_incorrect_vmcs_revision_id";
    case instruction_error::vmread_vmwrite_invalid_component: return "vmread_vmwrite_invalid_component";
    case instruction_error::vmwrite_readonly_component: return "vmwrite_readonly_component";
    case instruction_error::reserved_1: return "reserved_1";
    case instruction_error::vmxon_in_vmx_root_op: return "vmxon_in_vmx_root_op";
    case instruction_error::vmentry_invalid_vmcs_executive_pointer: return "vmentry_invalid_vmcs_executive_pointer";
    case instruction_error::vmentry_non_launched_executive_vmcs: return "vmentry_non_launched_executive_vmcs";
    case instruction_error::vmentry_executive_vmcs_ptr: return "vmentry_executive_vmcs_ptr";
    case instruction_error::vmcall_non_clear_vmcs: return "vmcall_non_clear_vmcs";
    case instruction_error::vmcall_invalid_vmexit_fields: return "vmcall_invalid_vmexit_fields";
    case instruction_error::reserved_2: return "reserved_2";
    case instruction_error::vmcall_invalid_mseg_revision_id: return "vmcall_invalid_mseg_revision_id";
    case instruction_error::vmxoff_dual_monitor: return "vmxoff_dual_monitor";
    case instruction_error::vmcall_invalid_smm_monitor: return "vmcall_invalid_smm_monitor";
    case instruction_error::vmentry_invalid_vm_execution_control: return "vmentry_invalid_vm_execution_control";
    case instruction_error::vmentry_mov_ss: return "vmentry_mov_ss";
    case instruction_error::reserved_3: return "reserved_3";
    case instruction_error::invept_invvpid_invalid_operand: return "invept_invvpid_invalid_operand";
  }

  return "";
}

}
