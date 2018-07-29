#pragma once
#include <cstdint>

namespace ia32::vmx {

struct exit_qualification_debug_exception
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

struct exit_qualification_pagefault
{
  union
  {
    uint64_t flags;
    uint64_t linear_address;
  };
};

struct exit_qualification_task_switch
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

struct exit_qualification_mov_cr
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

struct exit_qualification_mov_dr
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

struct exit_qualification_io_instruction
{
  enum
  {
    access_out = 0,
    access_in = 1
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

struct exit_qualification_apic_access
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

struct exit_qualification_ept_violation
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t data_read : 1;
      uint64_t data_write : 1;
      uint64_t instruction_fetch : 1;
      uint64_t entry_present : 1;
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

struct exit_qualification
{
  union
  {
    uint64_t flags;

    exit_qualification_debug_exception debug_exception;
    exit_qualification_pagefault       pagefault;
    exit_qualification_task_switch     task_switch;
    exit_qualification_mov_cr          mov_cr;
    exit_qualification_mov_dr          mov_dr;
    exit_qualification_io_instruction  io_instruction;
    exit_qualification_apic_access     apic_access;
    exit_qualification_ept_violation   ept_violation;
  };
};

}
