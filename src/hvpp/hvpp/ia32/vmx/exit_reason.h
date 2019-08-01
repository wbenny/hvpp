#pragma once
#include <cstdint>

namespace ia32::vmx {

enum class exit_reason : uint16_t
{
  exception_or_nmi              = 0x00000000,
  external_interrupt            = 0x00000001,
  triple_fault                  = 0x00000002,
  init_signal                   = 0x00000003,
  startup_ipi                   = 0x00000004,
  io_smi                        = 0x00000005,
  smi                           = 0x00000006,
  interrupt_window              = 0x00000007,
  nmi_window                    = 0x00000008,
  task_switch                   = 0x00000009,
  execute_cpuid                 = 0x0000000a,
  execute_getsec                = 0x0000000b,
  execute_hlt                   = 0x0000000c,
  execute_invd                  = 0x0000000d,
  execute_invlpg                = 0x0000000e,
  execute_rdpmc                 = 0x0000000f,
  execute_rdtsc                 = 0x00000010,
  execute_rsm_in_smm            = 0x00000011,
  execute_vmcall                = 0x00000012,
  execute_vmclear               = 0x00000013,
  execute_vmlaunch              = 0x00000014,
  execute_vmptrld               = 0x00000015,
  execute_vmptrst               = 0x00000016,
  execute_vmread                = 0x00000017,
  execute_vmresume              = 0x00000018,
  execute_vmwrite               = 0x00000019,
  execute_vmxoff                = 0x0000001a,
  execute_vmxon                 = 0x0000001b,
  mov_cr                        = 0x0000001c,
  mov_dr                        = 0x0000001d,
  execute_io_instruction        = 0x0000001e,
  execute_rdmsr                 = 0x0000001f,
  execute_wrmsr                 = 0x00000020,
  error_invalid_guest_state     = 0x00000021,
  error_msr_load                = 0x00000022,
  reserved_1                    = 0x00000023,
  execute_mwait                 = 0x00000024,
  monitor_trap_flag             = 0x00000025,
  reserved_2                    = 0x00000026,
  execute_monitor               = 0x00000027,
  execute_pause                 = 0x00000028,
  error_machine_check           = 0x00000029,
  reserved_3                    = 0x0000002a,
  tpr_below_threshold           = 0x0000002b,
  apic_access                   = 0x0000002c,
  virtualized_eoi               = 0x0000002d,
  gdtr_idtr_access              = 0x0000002e,
  ldtr_tr_access                = 0x0000002f,
  ept_violation                 = 0x00000030,
  ept_misconfiguration          = 0x00000031,
  execute_invept                = 0x00000032,
  execute_rdtscp                = 0x00000033,
  vmx_preemption_timer_expired  = 0x00000034,
  execute_invvpid               = 0x00000035,
  execute_wbinvd                = 0x00000036,
  execute_xsetbv                = 0x00000037,
  apic_write                    = 0x00000038,
  execute_rdrand                = 0x00000039,
  execute_invpcid               = 0x0000003a,
  execute_vmfunc                = 0x0000003b,
  execute_encls                 = 0x0000003c,
  execute_rdseed                = 0x0000003d,
  page_modification_log_full    = 0x0000003e,
  execute_xsaves                = 0x0000003f,
  execute_xrstors               = 0x00000040,
};

constexpr inline const char* to_string(exit_reason value) noexcept
{
  switch (value)
  {
    case exit_reason::exception_or_nmi: return "exception_or_nmi";
    case exit_reason::external_interrupt: return "external_interrupt";
    case exit_reason::triple_fault: return "triple_fault";
    case exit_reason::init_signal: return "init_signal";
    case exit_reason::startup_ipi: return "startup_ipi";
    case exit_reason::io_smi: return "io_smi";
    case exit_reason::smi: return "smi";
    case exit_reason::interrupt_window: return "interrupt_window";
    case exit_reason::nmi_window: return "nmi_window";
    case exit_reason::task_switch: return "task_switch";
    case exit_reason::execute_cpuid: return "execute_cpuid";
    case exit_reason::execute_getsec: return "execute_getsec";
    case exit_reason::execute_hlt: return "execute_hlt";
    case exit_reason::execute_invd: return "execute_invd";
    case exit_reason::execute_invlpg: return "execute_invlpg";
    case exit_reason::execute_rdpmc: return "execute_rdpmc";
    case exit_reason::execute_rdtsc: return "execute_rdtsc";
    case exit_reason::execute_rsm_in_smm: return "execute_rsm_in_smm";
    case exit_reason::execute_vmcall: return "execute_vmcall";
    case exit_reason::execute_vmclear: return "execute_vmclear";
    case exit_reason::execute_vmlaunch: return "execute_vmlaunch";
    case exit_reason::execute_vmptrld: return "execute_vmptrld";
    case exit_reason::execute_vmptrst: return "execute_vmptrst";
    case exit_reason::execute_vmread: return "execute_vmread";
    case exit_reason::execute_vmresume: return "execute_vmresume";
    case exit_reason::execute_vmwrite: return "execute_vmwrite";
    case exit_reason::execute_vmxoff: return "execute_vmxoff";
    case exit_reason::execute_vmxon: return "execute_vmxon";
    case exit_reason::mov_cr: return "mov_cr";
    case exit_reason::mov_dr: return "mov_dr";
    case exit_reason::execute_io_instruction: return "execute_io_instruction";
    case exit_reason::execute_rdmsr: return "execute_rdmsr";
    case exit_reason::execute_wrmsr: return "execute_wrmsr";
    case exit_reason::error_invalid_guest_state: return "error_invalid_guest_state";
    case exit_reason::error_msr_load: return "error_msr_load";
    case exit_reason::execute_mwait: return "execute_mwait";
    case exit_reason::monitor_trap_flag: return "monitor_trap_flag";
    case exit_reason::execute_monitor: return "execute_monitor";
    case exit_reason::execute_pause: return "execute_pause";
    case exit_reason::error_machine_check: return "error_machine_check";
    case exit_reason::tpr_below_threshold: return "tpr_below_threshold";
    case exit_reason::apic_access: return "apic_access";
    case exit_reason::virtualized_eoi: return "virtualized_eoi";
    case exit_reason::gdtr_idtr_access: return "gdtr_idtr_access";
    case exit_reason::ldtr_tr_access: return "ldtr_tr_access";
    case exit_reason::ept_violation: return "ept_violation";
    case exit_reason::ept_misconfiguration: return "ept_misconfiguration";
    case exit_reason::execute_invept: return "execute_invept";
    case exit_reason::execute_rdtscp: return "execute_rdtscp";
    case exit_reason::vmx_preemption_timer_expired: return "vmx_preemption_timer_expired";
    case exit_reason::execute_invvpid: return "execute_invvpid";
    case exit_reason::execute_wbinvd: return "execute_wbinvd";
    case exit_reason::execute_xsetbv: return "execute_xsetbv";
    case exit_reason::apic_write: return "apic_write";
    case exit_reason::execute_rdrand: return "execute_rdrand";
    case exit_reason::execute_invpcid: return "execute_invpcid";
    case exit_reason::execute_vmfunc: return "execute_vmfunc";
    case exit_reason::execute_encls: return "execute_encls";
    case exit_reason::execute_rdseed: return "execute_rdseed";
    case exit_reason::page_modification_log_full: return "page_modification_log_full";
    case exit_reason::execute_xsaves: return "execute_xsaves";
    case exit_reason::execute_xrstors: return "execute_xrstors";
  }

  return "";
}

}
