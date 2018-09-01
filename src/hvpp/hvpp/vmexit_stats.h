#pragma once
#include "vmexit.h"

#include "lib/bitmap.h"

#include <atomic>

namespace hvpp {

class vcpu_t;

//
// Simple VM-exit handler which performs statistics about VM-exits
// and also allows their tracing (by hvpp_trace()).
//

class vmexit_stats_handler
  : public vmexit_handler
{
  public:
    struct stats_t
    {
      void merge(const stats_t& other) noexcept;
      void dump() const noexcept;

      //
      // Counter for each VM-exit reason (ia32::vmx::exit_reason).
      // Currently the highest ID of exit reason is 65, which is
      // kind of unfortunate number.  We'll just round it to 80.
      //
      uint32_t vmexit[80];

      //
      // Counter for each exception vector (ia32::exception_vector).
      // This is subcategory of exit_reason::exception_or_nmi (0).
      //
      uint32_t expt_vector[20];

      //
      // Counter for each CPUID instruction:
      //   - cpuid_0:     CPUID with eax in range 0x0000'0000 - 0x0000'000F
      //   - cpuid_8:     CPUID with eax in range 0x8000'0000 - 0x8000'000F
      //   - cpuid_other: CPUID with other eax values
      // This is subcategory of exit_reason::execute_cpuid (10).
      //
      uint32_t cpuid_0[16];
      uint32_t cpuid_8[16];
      uint32_t cpuid_other;

      //
      // Counter for each MOV CR (from/to), CLTS and LMSW instruction.
      // Each array item in mov_from_cr/mov_to_cr represents counter for
      // specific CRn register.
      // This is subcategory of exit_reason::mov_cr (28).
      //
      uint32_t mov_from_cr[8];
      uint32_t mov_to_cr[8];
      uint32_t clts;
      uint32_t lmsw;

      //
      // Counter for each MOV DR (from/to) instruction.
      // Each array item in mov_from_dr/mov_to_dr represents counter
      // for specific DRn register.
      // This is subcategory of exit_reason::mov_dr (29).
      //
      uint32_t mov_from_dr[8];
      uint32_t mov_to_dr[8];

      //
      // Counter for each SGDT, SIDT, LGDT and LIDT instruction.
      // Each array item represents counter for specific instruction according
      // to the enum values defined in instruction_info_gdtr_idtr_access.
      // This is subcategory of exit_reason::gdtr_idtr_access (46).
      //
      uint32_t gdtr_idtr[4];

      //
      // Counter for each SLDT, STR, LLDT and LTR instruction.
      // Each array item represents counter for specific instruction according
      // to the enum values defined in instruction_info_ldtr_tr_access.
      // This is subcategory of exit_reason::ldtr_tr_access (47).
      //
      uint32_t ldtr_tr[4];

      //
      // Counters for each IN/OUT (INS/OUTS) instructions.
      // Each array item represents counter for specific I/O port.
      // This is subcategory of exit_reason::execute_io_instruction (30).
      //
      uint32_t io_in[0x10000];
      uint32_t io_out[0x10000];

      //
      // Counters for each RDMSR/WRMSR instructions.
      // Each array item represents counter for specific MSR number:
      //    rdmsr_0: MSR numbers in range 0x0000'0000 - 0x0000'1fff
      //    rdmsr_c: MSR numbers in range 0xc000'0000 - 0xc000'1fff
      //    rdmsr_other: MSR with other number values
      // The same applies with wrmsr_* members.
      // This is subcategory of exit_reason::execute_rdmsr (31)
      //                    and exit_reason::execute_wrmsr (32).
      //

      uint32_t rdmsr_0[0x2000];
      uint32_t rdmsr_c[0x2000];
      uint32_t rdmsr_other;
      uint32_t wrmsr_0[0x2000];
      uint32_t wrmsr_c[0x2000];
      uint32_t wrmsr_other;
    };

    vmexit_stats_handler() noexcept;
    void handle(vcpu_t& vp) noexcept override;
    void invoke_termination() noexcept override;

  private:
    void update_stats(vcpu_t& vp) noexcept;

    //
    // Array of stats (per VCPU).
    //
    stats_t* stats_;

    //
    // Bitmap of traced VM-exits.
    //
    bitmap vmexit_trace_bitmap_;
    uint8_t vmexit_trace_bitmap_buffer_[16];

    //
    // Count of terminated VCPUs.
    //
    std::atomic_uint32_t terminated_vcpu_count_;
};

}
