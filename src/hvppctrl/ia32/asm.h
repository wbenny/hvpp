#pragma once
#include <cstdint>

//
// ref: Vol2A[(INVPCID-Invalidate Process-Context Identifier)]
//

enum class invpcid_t : uint32_t
{
  individual_address                = 0x00000000,
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
  all_contexts_retaining_globals    = 0x00000003,
};

struct invpcid_desc_t
{
  uint64_t pcid : 12;
  uint64_t reserved : 52;
  uint64_t linear_address;
};

static_assert(sizeof(invpcid_desc_t) == 16);

enum class invept_t : uint32_t
{
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
};

enum class invvpid_t : uint32_t
{
  individual_address                = 0x00000000,
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
  single_context_retaining_globals  = 0x00000003,
};

struct invept_desc_t
{
  uint64_t ept_pointer;
  uint64_t reserved;
};

static_assert(sizeof(invept_desc_t) == 16);

struct invvpid_desc_t
{
  uint64_t vpid : 16;
  uint64_t reserved : 48;
  uint64_t linear_address;
};

static_assert(sizeof(invvpid_desc_t) == 16);

#include "win32/asm.h"
