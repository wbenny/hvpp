#pragma once
#include <cstdlib>

namespace ia32::vmx {

enum instruction_error : uint32_t
{
  no_error                                           =  0,
  vmcall_in_vmx_root_operation                       =  1,
  vmclear_invalid_physical_address                   =  2,
  vmclear_invalid_vmxon_pointer                      =  3,
  vmlauch_non_clear_vmcs                             =  4,
  vmresume_non_launched_vmcs                         =  5,
  vmresume_after_vmxoff                              =  6,
  vmentry_invalid_control_fields                     =  7,
  vmentry_invalid_host_state                         =  8,
  vmptrld_invalid_physical_address                   =  9,
  vmptrld_vmxon_pointer                              = 10,
  vmptrld_incorrect_vmcs_revision_id                 = 11,
  vmread_vmwrite_invalid_component                   = 12,
  vmwrite_readonly_component                         = 13,
  reserved_1                                         = 14,
  vmxon_in_vmx_root_op                               = 15,
  vmentry_invalid_vmcs_executive_pointer             = 16,
  vmentry_non_launched_executive_vmcs                = 17,
  vmentry_executive_vmcs_ptr                         = 18,
  vmcall_non_clear_vmcs                              = 19,
  vmcall_invalid_vmexit_fields                       = 20,
  reserved_2                                         = 21,
  vmcall_invalid_mseg_revision_id                    = 22,
  vmxoff_dual_monitor                                = 23,
  vmcall_invalid_smm_monitor                         = 24,
  vmentry_invalid_vm_execution_control               = 25,
  vmentry_mov_ss                                     = 26,
  reserved_3                                         = 27,
  invept_invvpid_invalid_operand                     = 28,
};


}
