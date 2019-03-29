#include "vmware.h"

bool
try_decode_io_instruction(
  const uint8_t* rip,
  int& access_type,
  int& size_of_access,
  bool& rep_prefixed
  ) noexcept
{
  enum op_prefix : uint8_t
  {
    op_prefix_size = 0x66,
    op_prefix_rep  = 0xf3,
  };

  enum op_instruction : uint8_t
  {
    op_insb        = 0x6c,
    op_insd        = 0x6d,
    op_outsb       = 0x6e,
    op_outsd       = 0x6f,
    op_in_al_dx    = 0xec,
    op_in_eax_dx   = 0xed,
    op_out_dx_al   = 0xee,
    op_out_dx_eax  = 0xef,
  };

  rep_prefixed = false;

  //
  // Skip instruction prefixes.
  //
  int size_of_access_adjust = 0;
  do switch (*rip) {
    case op_prefix_rep:  rep_prefixed          = true; break;
    case op_prefix_size: size_of_access_adjust = 2;    break;
    default: goto next;
  } while (rip++ || 1);

next:

  //
  // Check access type (0 = out, 1 = in).
  //
  switch (*rip)
  {
    case op_insb:
    case op_insd:
    case op_in_al_dx:
    case op_in_eax_dx:
      access_type = 1;
      break;
    case op_outsb:
    case op_outsd:
    case op_out_dx_al:
    case op_out_dx_eax:
      access_type = 0;
      break;

    default:
      goto error;
  }

  switch (*rip)
  {
    case op_insb:
    case op_outsb:
    case op_in_al_dx:
    case op_out_dx_al:
      size_of_access = 1;
      break;

    case op_insd:
    case op_outsd:
    case op_in_eax_dx:
    case op_out_dx_eax:
      size_of_access = 4;
      break;
  }

  size_of_access -= size_of_access_adjust;

  //
  // This is I/O instruction.
  //
  return true;

error:
  //
  // This is not I/O instruction.
  //
  return false;
}

bool
try_decode_io_instruction(
  const ia32::context_t& ctx,
  ia32::vmx::exit_qualification_io_instruction_t& exit_qualification
  ) noexcept
{
  int access_type;
  int size_of_access;
  bool rep_prefixed;

  const auto rip = reinterpret_cast<const uint8_t*>(ctx.rip);
  if (!try_decode_io_instruction(rip, access_type, size_of_access, rep_prefixed))
  {
    return false;
  }

  exit_qualification.flags = 0;       // Reset the whole value.

  exit_qualification.access_type      = access_type;
  exit_qualification.size_of_access   = size_of_access - 1;
  exit_qualification.rep_prefixed     = rep_prefixed;
  exit_qualification.operand_encoding = ia32::vmx::exit_qualification_io_instruction_t::op_encoding_dx;
  exit_qualification.port_number      = ctx.rdx & 0xffff;  // only lower bits.
  return true;
}
