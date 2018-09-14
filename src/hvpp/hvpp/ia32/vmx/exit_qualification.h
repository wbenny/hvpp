#pragma once
#include <cstdint>

namespace ia32::vmx {

struct exit_qualification_debug_exception_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t breakpoint_condition : 4;
      uint64_t reserved_1 : 9;
      uint64_t debug_register_access_detected : 1;
      uint64_t single_instruction : 1;
    };
  };
};

struct exit_qualification_task_switch_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t selector : 16;
      uint64_t reserved_1 : 14;
      uint64_t type : 2;
    };
  };
};

struct exit_qualification_mov_cr_t
{
  enum
  {
    access_to_cr = 0,
    access_from_cr = 1,
    access_clts = 2,
    access_lmsw = 3,
  };

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t cr_number : 4;
      uint64_t access_type : 2;
      uint64_t lmsw_operand_type : 1;
      uint64_t reserved_1 : 1;
      uint64_t gp_register : 4;
      uint64_t reserved_2 : 4;
      uint64_t lmsw_source_data : 16;
    };
  };
};

struct exit_qualification_mov_dr_t
{
  enum
  {
    access_to_dr = 0,
    access_from_dr = 1,
  };

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t dr_number : 3;
      uint64_t reserved_1 : 1;
      uint64_t access_type : 1;
      uint64_t reserved_2 : 3;
      uint64_t gp_register : 4;
    };
  };
};

struct exit_qualification_io_instruction_t
{
  enum
  {
    access_out = 0,
    access_in = 1,
  };

  enum
  {
    op_encoding_dx = 0,
    op_encoding_imm = 1,
  };

  union
  {
    uint64_t flags;

    struct
    {
      uint64_t size_of_access : 3;
      uint64_t access_type : 1;
      uint64_t string_instruction : 1;
      uint64_t rep_prefixed : 1;
      uint64_t operand_encoding : 1;
      uint64_t reserved_1 : 9;
      uint64_t port_number : 16;
    };
  };
};

struct exit_qualification_apic_access_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t page_offset : 12;
      uint64_t access_type : 4;
    };
  };
};

struct exit_qualification_ept_violation_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t data_read : 1;
      uint64_t data_write : 1;
      uint64_t data_execute : 1;
      uint64_t entry_read : 1;
      uint64_t entry_write : 1;
      uint64_t entry_execute : 1;
      uint64_t entry_execute_for_user_mode : 1;
      uint64_t valid_guest_linear_address : 1;
      uint64_t ept_translated_access : 1;
      uint64_t user_mode_linear_address : 1;
      uint64_t readable_writable_page : 1;
      uint64_t execute_disable_page : 1;
      uint64_t nmi_unblocking : 1;
    };
  };
};

struct exit_qualification_t
{
  union
  {
    uint64_t flags;

    //
    // For INVEPT, INVPCID, INVVPID, LGDT, LIDT, LLDT, LTR, SGDT, SIDT,
    // SLDT, STR, VMCLEAR, VMPTRLD, VMPTRST, VMREAD, VMWRITE, VMXON,
    // XRSTORS, and XSAVES, the exit qualification receives the value
    // of the instruction’s displacement field, which is sign-extended
    // to 64 bits if necessary (32 bits on processors that do not support
    // Intel 64 architecture).  If the instruction has no displacement
    // (for example, has a register operand), zero is stored into the
    // exit qualification.  On processors that support Intel 64 architecture,
    // an exception is made for RIP-relative addressing (used only in 64-bit
    // mode).  Such addressing causes an instruction to use an address
    // that is the sum of the displacement field and the value of RIP
    // that references the following instruction.  In this case, the
    // exit qualification is loaded with the sum of the displacement field
    // and the appropriate RIP value.
    // (ref: Vol3C[27.2.1(Basic VM-Exit Information)])
    //
    uint64_t                              displacement;

    //
    // For a page-fault exception, the exit qualification contains the
    // linear-address that caused the page fault.
    //
    // For INVLPG, the exit qualification contains the linear-address operand
    // of the instruction.
    //
    uint64_t                              linear_address;


    exit_qualification_debug_exception_t  debug_exception;
    exit_qualification_task_switch_t      task_switch;
    exit_qualification_mov_cr_t           mov_cr;
    exit_qualification_mov_dr_t           mov_dr;
    exit_qualification_io_instruction_t   io_instruction;
    exit_qualification_apic_access_t      apic_access;
    exit_qualification_ept_violation_t    ept_violation;
  };
};

}
