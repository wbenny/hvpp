#pragma once
#include "hvpp/ia32/memory.h"

#include "memory_mapper.h"

namespace mm
{
  using namespace ia32;

  //
  // Class for reading/writing process virtual memory.
  //

  class memory_translator
  {
    public:
      memory_translator() noexcept;
      ~memory_translator() noexcept;

      memory_translator(const memory_translator& other) noexcept = delete;
      memory_translator(memory_translator&& other) noexcept = delete;
      memory_translator& operator=(const memory_translator& other) noexcept = delete;
      memory_translator& operator=(memory_translator&& other) noexcept = delete;

      pa_t va_to_pa(va_t va) noexcept;
      pa_t va_to_pa(va_t va, cr3_t cr3) noexcept;

      va_t read(va_t va, cr3_t cr3, void* buffer, size_t size, bool ignore_errors = false) noexcept;
      va_t write(va_t va, cr3_t cr3, const void* buffer, size_t size, bool ignore_errors = false) noexcept;

    private:
      va_t read_write(va_t va, cr3_t cr3, void* buffer, size_t size, bool write, bool ignore_errors) noexcept;

      memory_mapper mapper_;
  };
}
