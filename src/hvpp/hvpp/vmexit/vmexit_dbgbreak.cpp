#include "vmexit_dbgbreak.h"

#include "hvpp/vcpu.h"

#include <iterator> // std::size()

#define hvpp_break_if(value)                                    \
  do                                                            \
  {                                                             \
    bool flag_clear = false;                                    \
    bool flag_set   = true;                                     \
    if (value.compare_exchange_strong(flag_set, flag_clear))    \
    {                                                           \
      __debugbreak();                                           \
    }                                                           \
  } while (0)

namespace hvpp {

auto vmexit_dbgbreak_handler::initialize() noexcept -> error_code_t
{
  memset(&storage_, 0, sizeof(storage_));

  //
  // Uncomment this to break on IN 0x64 instruction.
  // Breakpoints on specific VM-exit reasons can be enabled/disabled
  // via this structure.
  //
  // storage_.io_in[0x64] = true;
  //

  return error_code_t{};
}

void vmexit_dbgbreak_handler::destroy() noexcept
{

}

void vmexit_dbgbreak_handler::handle(vcpu_t& vp) noexcept
{
  auto exit_reason = vp.exit_reason();

  hvpp_break_if(storage_.vmexit[static_cast<int>(exit_reason)]);

  switch (exit_reason)
  {
    case vmx::exit_reason::exception_or_nmi:
      hvpp_break_if(storage_.expt_vector[static_cast<int>(vp.exit_interrupt_info().vector())]);
      break;

    case vmx::exit_reason::execute_cpuid:
      if (vp.exit_context().eax < (0x0000'0000u + vmexit_dbgbreak_storage_t::cpuid_0_max))
      {
        hvpp_break_if(storage_.cpuid_0[vp.exit_context().eax]);
      }
      else if (vp.exit_context().eax >= 0x8000'0000u &&
                vp.exit_context().eax < (0x8000'0000u + vmexit_dbgbreak_storage_t::cpuid_8_max))
      {
        hvpp_break_if(storage_.cpuid_8[vp.exit_context().eax - 0x8000'0000u]);
      }
      else
      {
        hvpp_break_if(storage_.cpuid_other);
      }
      break;

    case vmx::exit_reason::mov_cr:
      switch (vp.exit_qualification().mov_cr.access_type)
      {
        case vmx::exit_qualification_mov_cr_t::access_to_cr:
          hvpp_break_if(storage_.mov_to_cr[vp.exit_qualification().mov_cr.cr_number]);
          break;

        case vmx::exit_qualification_mov_cr_t::access_from_cr:
          hvpp_break_if(storage_.mov_from_cr[vp.exit_qualification().mov_cr.cr_number]);
          break;

        case vmx::exit_qualification_mov_cr_t::access_clts:
          hvpp_break_if(storage_.clts);
          break;

        case vmx::exit_qualification_mov_cr_t::access_lmsw:
          hvpp_break_if(storage_.lmsw);
          break;
      }
      break;

    case vmx::exit_reason::mov_dr:
      switch (vp.exit_qualification().mov_dr.access_type)
      {
        case vmx::exit_qualification_mov_dr_t::access_to_dr:
          hvpp_break_if(storage_.mov_to_dr[vp.exit_qualification().mov_dr.dr_number]);
          break;

        case vmx::exit_qualification_mov_dr_t::access_from_dr:
          hvpp_break_if(storage_.mov_from_dr[vp.exit_qualification().mov_dr.dr_number]);
          break;
      }
      break;

    case vmx::exit_reason::execute_io_instruction:
      switch (vp.exit_qualification().io_instruction.access_type)
      {
        case vmx::exit_qualification_io_instruction_t::access_out:
          hvpp_break_if(storage_.io_out[vp.exit_qualification().io_instruction.port_number]);
          break;

        case vmx::exit_qualification_io_instruction_t::access_in:
          hvpp_break_if(storage_.io_in[vp.exit_qualification().io_instruction.port_number]);
          break;
      }
      break;

    case vmx::exit_reason::execute_rdmsr:
      if (vp.exit_context().ecx <= 0x0000'1fffu)
      {
        hvpp_break_if(storage_.rdmsr_0[vp.exit_context().ecx]);
      }
      else if (vp.exit_context().ecx >= 0xc000'0000u &&
                vp.exit_context().ecx <= 0xc000'1fffu)
      {
        hvpp_break_if(storage_.rdmsr_c[vp.exit_context().ecx - 0xc000'0000u]);
      }
      else
      {
        hvpp_break_if(storage_.rdmsr_other);
      }
      break;

    case vmx::exit_reason::execute_wrmsr:
      if (vp.exit_context().ecx <= 0x0000'1fffu)
      {
        hvpp_break_if(storage_.wrmsr_0[vp.exit_context().ecx]);
      }
      else if (vp.exit_context().ecx >= 0xc000'0000u &&
                vp.exit_context().ecx <= 0xc000'1fffu)
      {
        hvpp_break_if(storage_.wrmsr_c[vp.exit_context().ecx - 0xc000'0000u]);
      }
      else
      {
        hvpp_break_if(storage_.wrmsr_other);
      }
      break;

    case vmx::exit_reason::gdtr_idtr_access:
      hvpp_break_if(storage_.gdtr_idtr[vp.exit_instruction_info().gdtr_idtr_access.instruction]);
      break;

    case vmx::exit_reason::ldtr_tr_access:
      hvpp_break_if(storage_.ldtr_tr[vp.exit_instruction_info().ldtr_tr_access.instruction]);
      break;
  }
}

}
