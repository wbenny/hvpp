#pragma once

#include <cstdint>

namespace ia32 {

struct cpuid_eax_01
{
  union
  {
    struct
    {
      uint32_t cpu_info[4];
    };

    struct
    {
      uint32_t eax;
      uint32_t ebx;
      uint32_t ecx;
      uint32_t edx;
    };

    struct
    {
      union
      {
        uint32_t flags;

        struct
        {
          uint32_t stepping_id : 4;
          uint32_t model : 4;
          uint32_t family_id : 4;
          uint32_t processor_type : 2;
          uint32_t reserved1 : 2;
          uint32_t extended_model_id : 4;
          uint32_t extended_family_id : 8;
          uint32_t reserved2 : 4;
        };
      } version_information;

      union
      {
        uint32_t flags;

        struct
        {
          uint32_t brand_index : 8;
          uint32_t clflush_line_size : 8;
          uint32_t max_addressable_ids : 8;
          uint32_t initial_apic_id : 8;
        };
      } additional_information;

      union
      {
        uint32_t flags;

        struct
        {
          uint32_t streaming_simd_extensions_3 : 1;
          uint32_t pclmulqdq_instruction : 1;
          uint32_t ds_area_64bit_layout : 1;
          uint32_t monitor_mwait_instruction : 1;
          uint32_t cpl_qualified_debug_store : 1;
          uint32_t virtual_machine_extensions : 1;
          uint32_t safer_mode_extensions : 1;
          uint32_t enhanced_intel_speedstep_technology : 1;
          uint32_t thermal_monitor_2 : 1;
          uint32_t supplemental_streaming_simd_extensions_3 : 1;
          uint32_t l1_context_id : 1;
          uint32_t silicon_debug : 1;
          uint32_t fma_extensions : 1;
          uint32_t cmpxchg16b_instruction : 1;
          uint32_t xtpr_update_control : 1;
          uint32_t perfmon_and_debug_capability : 1;
          uint32_t reserved1 : 1;
          uint32_t process_context_identifiers : 1;
          uint32_t direct_cache_access : 1;
          uint32_t sse41_support : 1;
          uint32_t sse42_support : 1;
          uint32_t x2apic_support : 1;
          uint32_t movbe_instruction : 1;
          uint32_t popcnt_instruction : 1;
          uint32_t tsc_deadline : 1;
          uint32_t aesni_instruction_extensions : 1;
          uint32_t xsave_xrstor_instruction : 1;
          uint32_t osx_save : 1;
          uint32_t avx_support : 1;
          uint32_t half_precision_conversion_instructions : 1;
          uint32_t rdrand_instruction : 1;
          uint32_t hypervisor_present : 1;
        };
      } feature_information_ecx;

      union
      {
        uint32_t flags;

        struct
        {
          uint32_t floating_point_unit_on_chip : 1;
          uint32_t virtual_8086_mode_enhancements : 1;
          uint32_t debugging_extensions : 1;
          uint32_t page_size_extension : 1;
          uint32_t timestamp_counter : 1;
          uint32_t rdmsr_wrmsr_instructions : 1;
          uint32_t physical_address_extension : 1;
          uint32_t machine_check_exception : 1;
          uint32_t cmpxchg8b : 1;
          uint32_t apic_on_chip : 1;
          uint32_t reserved1 : 1;
          uint32_t sysenter_sysexit_instructions : 1;
          uint32_t memory_type_range_registers : 1;
          uint32_t page_global_bit : 1;
          uint32_t machine_check_architecture : 1;
          uint32_t conditional_move_instructions : 1;
          uint32_t page_attribute_table : 1;
          uint32_t page_size_extension_36bit : 1;
          uint32_t processor_serial_number : 1;
          uint32_t clflush : 1;
          uint32_t reserved2 : 1;
          uint32_t debug_store : 1;
          uint32_t thermal_control_msrs_for_acpi : 1;
          uint32_t mmx_support : 1;
          uint32_t fxsave_fxrstor_instructions : 1;
          uint32_t sse_support : 1;
          uint32_t sse2_support : 1;
          uint32_t self_snoop : 1;
          uint32_t hyper_threading_technology : 1;
          uint32_t thermal_monitor : 1;
          uint32_t reserved3 : 1;
          uint32_t pending_break_enable : 1;
        };
      } feature_information_edx;
    };
  };
};

}
