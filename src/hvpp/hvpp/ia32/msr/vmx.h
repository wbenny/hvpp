#pragma once
#include "../ia32/arch.h" // cr0_t, cr4_t

#include <cstdint>

namespace ia32::msr {

struct vmx_true_ctls_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint32_t allowed_0_settings;
      uint32_t allowed_1_settings;
    };
  };
};

struct vmx_basic_t
{
  static constexpr uint32_t msr_id = 0x00000480;
  using result_type = vmx_basic_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t vmcs_revision_id : 31;
      uint64_t reserved_1 : 1;
      uint64_t vmcs_size_in_bytes : 13;
      uint64_t reserved_2 : 3;
      uint64_t vmcs_physical_address_width : 1;
      uint64_t dual_monitor : 1;
      uint64_t memory_type : 4;
      uint64_t ins_outs_vmexit_information : 1;
      uint64_t true_controls : 1;
    };
  };
};

struct vmx_pinbased_ctls_t
{
  static constexpr uint32_t msr_id = 0x00000481;
  using result_type = vmx_pinbased_ctls_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t external_interrupt_exiting : 1;
      uint64_t reserved_1 : 2;
      uint64_t nmi_exiting : 1;
      uint64_t reserved_2 : 1;
      uint64_t virtual_nmis : 1;
      uint64_t activate_vmx_preemption_timer : 1;
      uint64_t process_posted_interrupts : 1;
    };
  };
};

struct vmx_procbased_ctls_t
{
  static constexpr uint32_t msr_id = 0x00000482;
  using result_type = vmx_procbased_ctls_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 2;
      uint64_t interrupt_window_exiting : 1;
      uint64_t use_tsc_offsetting : 1;
      uint64_t reserved_2 : 3;
      uint64_t hlt_exiting : 1;
      uint64_t reserved_3 : 1;
      uint64_t invlpg_exiting : 1;
      uint64_t mwait_exiting : 1;
      uint64_t rdpmc_exiting : 1;
      uint64_t rdtsc_exiting : 1;
      uint64_t reserved_4 : 2;
      uint64_t cr3_load_exiting : 1;
      uint64_t cr3_store_exiting : 1;
      uint64_t reserved_5 : 2;
      uint64_t cr8_load_exiting : 1;
      uint64_t cr8_store_exiting : 1;
      uint64_t use_tpr_shadow : 1;
      uint64_t nmi_window_exiting : 1;
      uint64_t mov_dr_exiting : 1;
      uint64_t unconditional_io_exiting : 1;
      uint64_t use_io_bitmaps : 1;
      uint64_t reserved_6 : 1;
      uint64_t monitor_trap_flag : 1;
      uint64_t use_msr_bitmaps : 1;
      uint64_t monitor_exiting : 1;
      uint64_t pause_exiting : 1;
      uint64_t activate_secondary_controls : 1;
    };
  };
};

struct vmx_exit_ctls_t
{
  static constexpr uint32_t msr_id = 0x00000483;
  using result_type = vmx_exit_ctls_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 2;
      uint64_t save_debug_controls : 1;
      uint64_t reserved_2 : 6;
      uint64_t ia32e_mode_host : 1;
      uint64_t reserved_3 : 2;
      uint64_t load_ia32_perf_global_ctrl : 1;
      uint64_t reserved_4 : 2;
      uint64_t acknowledge_interrupt_on_exit : 1;
      uint64_t reserved_5 : 2;
      uint64_t save_ia32_pat : 1;
      uint64_t load_ia32_pat : 1;
      uint64_t save_ia32_efer : 1;
      uint64_t load_ia32_efer : 1;
      uint64_t save_vmx_preemption_timer_value : 1;
      uint64_t clear_ia32_bndcfgs : 1;
      uint64_t conceal_vmx_from_pt : 1;
    };
  };
};

struct vmx_entry_ctls_t
{
  static constexpr uint32_t msr_id = 0x00000484;
  using result_type = vmx_entry_ctls_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t reserved_1 : 2;
      uint64_t load_debug_controls : 1;
      uint64_t reserved_2 : 6;
      uint64_t ia32e_mode_guest : 1;
      uint64_t entry_to_smm : 1;
      uint64_t deactivate_dual_monitor_treatment : 1;
      uint64_t reserved_3 : 1;
      uint64_t load_ia32_perf_global_ctrl : 1;
      uint64_t load_ia32_pat : 1;
      uint64_t load_ia32_efer : 1;
      uint64_t load_ia32_bndcfgs : 1;
      uint64_t conceal_vmx_from_pt : 1;
    };
  };
};

struct vmx_misc_t
{
  static constexpr uint32_t msr_id = 0x00000485;
  using result_type = vmx_misc_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t preemption_timer_tsc_relationship : 5;
      uint64_t store_efer_lma_on_vmexit : 1;
      uint64_t activity_states : 3;
      uint64_t reserved_1 : 5;
      uint64_t intel_pt_available_in_vmx : 1;
      uint64_t rdmsr_can_read_ia32_smbase_msr_in_smm : 1;
      uint64_t cr3_target_count : 9;
      uint64_t max_number_of_msr : 3;
      uint64_t smm_monitor_ctl_b2 : 1;
      uint64_t vmwrite_vmexit_info : 1;
      uint64_t zero_length_instruction_vmentry_injection : 1;
      uint64_t reserved_2 : 1;
      uint64_t mseg_id : 32;
    };
  };
};

struct vmx_cr0_fixed0_t
{
  static constexpr uint32_t msr_id = 0x00000486;
  using result_type = cr0_t;
};

struct vmx_cr0_fixed1_t
{
  static constexpr uint32_t msr_id = 0x00000487;
  using result_type = cr0_t;
};

struct vmx_cr4_fixed0_t
{
  static constexpr uint32_t msr_id = 0x00000488;
  using result_type = cr4_t;
};

struct vmx_cr4_fixed1_t
{
  static constexpr uint32_t msr_id = 0x00000489;
  using result_type = cr4_t;
};

struct vmx_procbased_ctls2_t
{
  static constexpr uint32_t msr_id = 0x0000048B;
  using result_type = vmx_procbased_ctls2_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t virtualize_apic_accesses : 1;
      uint64_t enable_ept : 1;
      uint64_t descriptor_table_exiting : 1;
      uint64_t enable_rdtscp : 1;
      uint64_t virtualize_x2apic_mode : 1;
      uint64_t enable_vpid : 1;
      uint64_t wbinvd_exiting : 1;
      uint64_t unrestricted_guest : 1;
      uint64_t apic_register_virtualization : 1;
      uint64_t virtual_interrupt_delivery : 1;
      uint64_t pause_loop_exiting : 1;
      uint64_t rdrand_exiting : 1;
      uint64_t enable_invpcid : 1;
      uint64_t enable_vm_functions : 1;
      uint64_t vmcs_shadowing : 1;
      uint64_t enable_encls_exiting : 1;
      uint64_t rdseed_exiting : 1;
      uint64_t enable_pml : 1;
      uint64_t ept_violation_ve : 1;
      uint64_t conceal_vmx_from_pt : 1;
      uint64_t enable_xsaves : 1;
      uint64_t reserved_1 : 1;
      uint64_t mode_based_execute_control_for_ept : 1;
      uint64_t reserved_2 : 2;
      uint64_t use_tsc_scaling : 1;
    };
  };
};

struct vmx_ept_vpid_cap_t
{
  static constexpr uint32_t msr_id = 0x0000048C;
  using result_type = vmx_ept_vpid_cap_t;

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t execute_only_pages : 1;
      uint64_t reserved_1 : 5;
      uint64_t page_walk_length_4 : 1;
      uint64_t reserved_2 : 1;
      uint64_t memory_type_uncacheable : 1;
      uint64_t reserved_3 : 5;
      uint64_t memory_type_write_back : 1;
      uint64_t reserved_4 : 1;
      uint64_t pde_2mb_pages : 1;
      uint64_t pdpte_1gb_pages : 1;
      uint64_t reserved_5 : 2;
      uint64_t invept : 1;
      uint64_t ept_accessed_and_dirty_flags : 1;
      uint64_t advanced_vmexit_ept_violations_information : 1;
      uint64_t reserved_6 : 2;
      uint64_t invept_single_context : 1;
      uint64_t invept_all_contexts : 1;
      uint64_t reserved_7 : 5;
      uint64_t invvpid : 1;
      uint64_t reserved_8 : 7;
      uint64_t invvpid_individual_address : 1;
      uint64_t invvpid_single_context : 1;
      uint64_t invvpid_all_contexts : 1;
      uint64_t invvpid_single_context_retain_globals : 1;
    };
  };
};

}
