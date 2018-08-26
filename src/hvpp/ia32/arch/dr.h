#pragma once
#include <cstdint>

namespace ia32 {

struct dr0_t { uint64_t flags; };
struct dr1_t { uint64_t flags; };
struct dr2_t { uint64_t flags; };
struct dr3_t { uint64_t flags; };
struct dr4_t { uint64_t flags; };
struct dr5_t { uint64_t flags; };

struct dr6_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t breakpoint_condition : 4;
      uint64_t reserved_1 : 8; // always 1
      uint64_t reserved_2 : 1; // always 0
      uint64_t debug_register_access_detected : 1;
      uint64_t single_instruction : 1;
      uint64_t task_switch : 1;
      uint64_t restricted_transactional_memory : 1;
      uint64_t reserved_3 : 15; // always 1
    };
  };
};

struct dr7_t
{
  union
  {
    uint64_t flags;

    struct
    {
      uint64_t local_breakpoint_0 : 1;
      uint64_t global_breakpoint_0 : 1;
      uint64_t local_breakpoint_1 : 1;
      uint64_t global_breakpoint_1 : 1;
      uint64_t local_breakpoint_2 : 1;
      uint64_t global_breakpoint_2 : 1;
      uint64_t local_breakpoint_3 : 1;
      uint64_t global_breakpoint_3 : 1;
      uint64_t local_exact_breakpoint : 1;
      uint64_t global_exact_breakpoint : 1;
      uint64_t reserved_1 : 1; // always 1
      uint64_t restricted_transactional_memory : 1;
      uint64_t reserved_2 : 1; // always 0
      uint64_t general_detect : 1;
      uint64_t reserved_3 : 2; // always 0
      uint64_t read_write_0 : 2;
      uint64_t length_0 : 2;
      uint64_t read_write_1 : 2;
      uint64_t length_1 : 2;
      uint64_t read_write_2 : 2;
      uint64_t length_2 : 2;
      uint64_t read_write_3 : 2;
      uint64_t length_3 : 2;
    };
  };
};

}
