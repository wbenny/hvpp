#include "vmexit_stats.h"
#include "vcpu.h"

#include "ia32/vmx.h"
#include "lib/log.h"
#include "lib/mp.h" // mp::cpu_index()

#include <iterator> // std::size()

#define hv_trace_if_enabled(format, ...)                          \
  do                                                              \
  {                                                               \
    if (vmexit_trace_bitmap_.test(static_cast<int>(exit_reason))) \
    {                                                             \
      hvpp_trace(format, __VA_ARGS__);                            \
    }                                                             \
  } while (0)

namespace hvpp {

vmexit_stats_handler::vmexit_stats_handler() noexcept
  : stats_()
  , vmexit_trace_bitmap_(vmexit_trace_bitmap_buffer_, 128)
{
  //
  // Trace all VM-exit reasons.
  // Tracing of specific exit reasons can be enabled/disabled via this bitmap.
  //
  vmexit_trace_bitmap_.set();
  // vmexit_trace_bitmap_.clear(static_cast<int>(vmx::exit_reason::exception_or_nmi));
}

void vmexit_stats_handler::handle(vcpu_t& vp) noexcept
{
  update_stats(vp);
  vmexit_handler::handle(vp);
}

void vmexit_stats_handler::invoke_termination() noexcept
{
  vmexit_handler::invoke_termination();

  //
  // Handler saves statistics for all VCPUs but invoke_termination() is called
  // per each VCPU, so it makes sense to call this function just once.
  //
  if (mp::cpu_index() == 0)
  {
    stats_.dump();
  }
}

const vmexit_stats_handler::stats_t& vmexit_stats_handler::stats() const noexcept
{
  return stats_;
}

void vmexit_stats_handler::update_stats(vcpu_t& vp) noexcept
{
  auto exit_reason = vp.exit_reason();
  stats_.vmexit[static_cast<int>(exit_reason)] += 1;

  switch (exit_reason)
  {
    case vmx::exit_reason::exception_or_nmi:
      switch (vp.exit_interrupt_info().type())
      {
        case vmx::interrupt_type::hardware_exception:
        case vmx::interrupt_type::software_exception:
          stats_.expt_vector[static_cast<int>(vp.exit_interrupt_info().vector())] += 1;

          hv_trace_if_enabled(
            "exit_reason::exception_or_nmi: %s",
            exception_vector_to_string(vp.exit_interrupt_info().vector()));
          break;
      }
      break;

    case vmx::exit_reason::execute_cpuid:
      if (vp.exit_context().eax <= 0x0000'000Fu)
      {
        stats_.cpuid_0[vp.exit_context().eax] += 1;
      }
      else if (vp.exit_context().eax >= 0x8000'0000u &&
               vp.exit_context().eax <= 0x8000'000fu)
      {
        stats_.cpuid_8[vp.exit_context().eax - 0x8000'0000u] += 1;
      }
      else
      {
        stats_.cpuid_other += 1;
      }

      hv_trace_if_enabled("exit_reason::execute_cpuid: 0x%08x", vp.exit_context().eax);
      break;

    case vmx::exit_reason::execute_invd:
      hv_trace_if_enabled("exit_reason::execute_invd");
      break;

    case vmx::exit_reason::execute_invlpg:
      hv_trace_if_enabled("exit_reason::execute_invlpg: 0x%p", vp.exit_qualification().linear_address);
      break;

    case vmx::exit_reason::execute_rdtsc:
      hv_trace_if_enabled("exit_reason::execute_rdtsc");
      break;

    case vmx::exit_reason::mov_cr:
      switch (vp.exit_qualification().mov_cr.access_type)
      {
        case vmx::exit_qualification_mov_cr_t::access_to_cr:
          stats_.mov_to_cr[vp.exit_qualification().mov_cr.cr_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::mov_cr: (to_cr%u) 0x%p",
            vp.exit_qualification().mov_cr.cr_number,
            vp.exit_context().gp_register[vp.exit_qualification().mov_cr.gp_register]);
          break;

        case vmx::exit_qualification_mov_cr_t::access_from_cr:
          stats_.mov_from_cr[vp.exit_qualification().mov_cr.cr_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::mov_cr: (from_cr%u) 0x%p",
            vp.exit_qualification().mov_cr.cr_number,
            vp.exit_context().gp_register[vp.exit_qualification().mov_cr.gp_register]);
          break;

        case vmx::exit_qualification_mov_cr_t::access_clts:
          stats_.clts += 1;

          hv_trace_if_enabled("exit_reason::mov_cr: (clts)");
          break;

        case vmx::exit_qualification_mov_cr_t::access_lmsw:
          stats_.lmsw += 1;

          hv_trace_if_enabled("exit_reason::mov_cr: (lmsw)");
          break;
      }
      break;

    case vmx::exit_reason::mov_dr:
      switch (vp.exit_qualification().mov_dr.access_type)
      {
        case vmx::exit_qualification_mov_dr_t::access_to_dr:
          stats_.mov_to_dr[vp.exit_qualification().mov_dr.dr_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::mov_dr: (to_dr%u) 0x%p",
            vp.exit_qualification().mov_dr.dr_number,
            vp.exit_context().gp_register[vp.exit_qualification().mov_dr.gp_register]);
          break;

        case vmx::exit_qualification_mov_dr_t::access_from_dr:
          stats_.mov_from_dr[vp.exit_qualification().mov_dr.dr_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::mov_dr: (from_dr%u) 0x%p",
            vp.exit_qualification().mov_dr.dr_number,
            vp.exit_context().gp_register[vp.exit_qualification().mov_dr.gp_register]);
          break;
      }
      break;

    case vmx::exit_reason::execute_io_instruction:
      switch (vp.exit_qualification().io_instruction.access_type)
      {
        case vmx::exit_qualification_io_instruction_t::access_out:
          stats_.io_out[vp.exit_qualification().io_instruction.port_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::execute_io_instruction: out 0x%04x",
            vp.exit_qualification().io_instruction.port_number);
          break;

        case vmx::exit_qualification_io_instruction_t::access_in:
          stats_.io_in[vp.exit_qualification().io_instruction.port_number] += 1;

          hv_trace_if_enabled(
            "exit_reason::execute_io_instruction: in 0x%04x",
            vp.exit_qualification().io_instruction.port_number);
          break;
      }
      break;

    case vmx::exit_reason::execute_rdmsr:
      if (vp.exit_context().ecx <= 0x0000'1fffu)
      {
        stats_.rdmsr_0[vp.exit_context().ecx] += 1;
      }
      else if (vp.exit_context().ecx >= 0xc000'0000u &&
               vp.exit_context().ecx <= 0xc000'1fffu)
      {
        stats_.rdmsr_c[vp.exit_context().ecx - 0xc000'0000u] += 1;
      }
      else
      {
        stats_.rdmsr_other += 1;
      }
      hv_trace_if_enabled("exit_reason::execute_rdmsr: 0x%08x", vp.exit_context().ecx);
      break;

    case vmx::exit_reason::execute_wrmsr:
      if (vp.exit_context().ecx <= 0x0000'1fffu)
      {
        stats_.wrmsr_0[vp.exit_context().ecx] += 1;
      }
      else if (vp.exit_context().ecx >= 0xc000'0000u &&
               vp.exit_context().ecx <= 0xc000'1fffu)
      {
        stats_.wrmsr_c[vp.exit_context().ecx - 0xc000'0000u] += 1;
      }
      else
      {
        stats_.wrmsr_other += 1;
      }

      hv_trace_if_enabled("exit_reason::execute_wrmsr: 0x%08x", vp.exit_context().ecx);
      break;

    case vmx::exit_reason::gdtr_idtr_access:
      stats_.gdtr_idtr[vp.exit_instruction_info().gdtr_idtr_access.instruction] += 1;

      hv_trace_if_enabled(
        "exit_reason::gdtr_idtr_access: %s",
        vmx::instruction_info_gdtr_idtr_to_string(vp.exit_instruction_info().gdtr_idtr_access.instruction));
      break;

    case vmx::exit_reason::ldtr_tr_access:
      stats_.ldtr_tr[vp.exit_instruction_info().ldtr_tr_access.instruction] += 1;

      hv_trace_if_enabled(
        "exit_reason::ldtr_tr_access: %s",
        vmx::instruction_info_ldtr_tr_to_string(vp.exit_instruction_info().ldtr_tr_access.instruction));
      break;

    case vmx::exit_reason::ept_violation:
      //
      // Do not trace.
      //
      break;

    case vmx::exit_reason::execute_rdtscp:
      hv_trace_if_enabled("exit_reason::execute_rdtscp");
      break;

    case vmx::exit_reason::execute_wbinvd:
      hv_trace_if_enabled("exit_reason::execute_wbinvd");
      break;

    case vmx::exit_reason::execute_xsetbv:
      hv_trace_if_enabled("exit_reason::execute_xsetbv: [0x%08x] -> %p",
        vp.exit_context().ecx,
        vp.exit_context().rdx << 32 | vp.exit_context().rax);
      break;

    case vmx::exit_reason::execute_invpcid:
      hv_trace_if_enabled("exit_reason::execute_invpcid");
      break;
  }
}

void vmexit_stats_handler::stats_t::dump() const noexcept
{
  hvpp_info("VMEXIT statistics");
  for (uint32_t exit_reason_index = 0; exit_reason_index < std::size(vmexit); ++exit_reason_index)
  {
    if (vmexit[exit_reason_index] > 0)
    {
      hvpp_info("  %s: %u",
        vmx::exit_reason_to_string(static_cast<vmx::exit_reason>(exit_reason_index)),
        vmexit[exit_reason_index]);

      switch (static_cast<vmx::exit_reason>(exit_reason_index))
      {
        case vmx::exit_reason::exception_or_nmi:
          for (uint32_t i = 0; i < std::size(expt_vector); ++i)
          {
            const char* expt_vector_string = exception_vector_to_string(static_cast<exception_vector>(i));
            (void)(expt_vector_string);
            if (expt_vector[i] > 0)
            {
              hvpp_info("    %s: %u", expt_vector_string, expt_vector[i]);
            }
          }
          break;

        case vmx::exit_reason::execute_cpuid:
          for (uint32_t i = 0; i < std::size(cpuid_0); ++i)
          {
            if (cpuid_0[i] > 0)
            {
              hvpp_info("    0x%08x: %u", i, cpuid_0[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(cpuid_8); ++i)
          {
            if (cpuid_8[i] > 0)
            {
              hvpp_info("    0x%08x: %u", i + 0x8000'0000u, cpuid_8[i]);
            }
          }

          if (cpuid_other > 0)
          {
            hvpp_info("    0x(OTHER): %u", cpuid_other);
          }
          break;

        case vmx::exit_reason::mov_cr:
          for (uint32_t i = 0; i < std::size(mov_from_cr); ++i)
          {
            if (mov_from_cr[i] > 0)
            {
              hvpp_info("    mov_from_cr[%i]: %u", i, mov_from_cr[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(mov_to_cr); ++i)
          {
            if (mov_to_cr[i] > 0)
            {
              hvpp_info("    mov_to_cr[%i]: %u", i, mov_to_cr[i]);
            }
          }

          if (lmsw > 0)
          {
            hvpp_info("    clts: %u", clts);
          }

          if (lmsw > 0)
          {
            hvpp_info("    lmsw: %u", lmsw);
          }
          break;

        case vmx::exit_reason::mov_dr:
          for (uint32_t i = 0; i < std::size(mov_from_dr); ++i)
          {
            if (mov_from_dr[i] > 0)
            {
              hvpp_info("    mov_from_dr[%i]: %u", i, mov_from_dr[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(mov_to_dr); ++i)
          {
            if (mov_to_dr[i] > 0)
            {
              hvpp_info("    mov_to_dr[%i]: %u", i, mov_to_dr[i]);
            }
          }
          break;

        case vmx::exit_reason::gdtr_idtr_access:
          for (uint32_t i = 0; i < std::size(gdtr_idtr); ++i)
          {
            if (gdtr_idtr[i] > 0)
            {
              hvpp_info("    %s: %u", vmx::instruction_info_gdtr_idtr_to_string(i), gdtr_idtr[i]);
            }
          }
          break;

        case vmx::exit_reason::ldtr_tr_access:
          for (uint32_t i = 0; i < std::size(ldtr_tr); ++i)
          {
            if (ldtr_tr[i] > 0)
            {
              hvpp_info("    %s: %u", vmx::instruction_info_ldtr_tr_to_string(i), ldtr_tr[i]);
            }
          }
          break;

        case vmx::exit_reason::execute_io_instruction:
          for (uint32_t i = 0; i < std::size(io_in); ++i)
          {
            if (io_in[i] > 0)
            {
              hvpp_info("    in: %u", i, io_in[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(io_out); ++i)
          {
            if (io_out[i] > 0)
            {
              hvpp_info("    out: %u", i, io_out[i]);
            }
          }
          break;

        case vmx::exit_reason::execute_rdmsr:
          for (uint32_t i = 0; i < std::size(rdmsr_0); ++i)
          {
            if (rdmsr_0[i])
            {
              hvpp_info("    0x%08x: %u", i, rdmsr_0[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(rdmsr_c); ++i)
          {
            if (rdmsr_c[i])
            {
              hvpp_info("    0x%08x: %u", i + 0xc000'0000u, rdmsr_c[i]);
            }
          }

          if (rdmsr_other > 0)
          {
            hvpp_info("    (OTHER): %u", rdmsr_other);
          }
          break;

        case vmx::exit_reason::execute_wrmsr:
          for (uint32_t i = 0; i < std::size(wrmsr_0); ++i)
          {
            if (wrmsr_0[i])
            {
              hvpp_info("    0x%08x: %u", i, wrmsr_0[i]);
            }
          }

          for (uint32_t i = 0; i < std::size(wrmsr_c); ++i)
          {
            if (wrmsr_c[i])
            {
              hvpp_info("    0x%08x: %u", i + 0xc000'0000u, wrmsr_c[i]);
            }
          }

          if (wrmsr_other > 0)
          {
            hvpp_info("    (OTHER): %u", wrmsr_other);
          }
          break;
      }
    }
  }
}

}
