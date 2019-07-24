#pragma once
#include "hvpp/ia32/arch.h"
#include "hvpp/ia32/vmx/exit_qualification.h"

#include <cstdint>

//
// When running in VMWare, emulating IN/OUT instructions becomes complicated.
//
// Just setting up custom I/O VM-exit instruction handler won't work here,
// because VMWare Tools execute I/O instructions in user mode.  Intel Manual
// says that #GP due to I/O executed in CPL > 0 has priority over VM-exit,
// even if unconditional I/O exiting is enabled.
// So in the moment when interception of 0x5658 and 0x5659 I/O ports
// is established, they won't actually reach the VM-exit I/O handler at all,
// because #GPs will be suddenly raised (no matter if you catching #GP
// via exception bitmap or not).
//
// Custom handler for #GP and check if the guest RIP points to any I/O
// instruction solves this problem.  This can be done via really simple
// function - no complex disassembler is needed.  Then we have to emulate
// the I/O instruction ourselves.  This involves another gotcha - this I/O
// must be emulated with [ RAX, RBX, RCX, RDX, RSI, RDI, RFLAGS.DF ] set
// according to the guest register state.  Which means that __in*/__out*
// intrinsics won't help us (Intel Manual doesn't say anything about I/O
// instructions changing these registers).  This must be solved with custom
// ASM function (see vmware.asm).  Also, we must not forget to write back
// those mentioned registers back to the guest state (except the RFLAGS.DF),
// because VMWare uses them for both input and output.
//
// After that the current instruction MUST be skipped AND the #GP must NOT
// be reinjected.  This is obvious - we want to continue execution after
// the IN/OUT instruction like nothing unusual happened.
//
// When all of this is done, VMWare Tools continue to work.
//

extern "C"
int
ia32_asm_io_with_context(
  ia32::vmx::exit_qualification_io_instruction_t exit_qualification,
  ia32::context_t& context
  ) noexcept;

bool
try_decode_io_instruction(
  const ia32::context_t& ctx,
  ia32::vmx::exit_qualification_io_instruction_t& exit_qualification
  ) noexcept;
