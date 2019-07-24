#pragma once
#include "hvpp/ia32/memory.h"

namespace mm
{
  using namespace ia32;

  namespace detail
  {
    void* mapper_allocate(size_t size) noexcept;
    void  mapper_free(void* va) noexcept;
  }

  //
  // Class for reading/writing physical memory.
  //

  class memory_mapper
  {
    public:
      memory_mapper() noexcept;
      ~memory_mapper() noexcept;

      memory_mapper(const memory_mapper& other) noexcept = delete;
      memory_mapper(memory_mapper&& other) noexcept = delete;
      memory_mapper& operator=(const memory_mapper& other) noexcept = delete;
      memory_mapper& operator=(memory_mapper&& other) noexcept = delete;

      void* map(pa_t pa) noexcept;
      void  unmap() noexcept;

      void  read(pa_t pa, void* buffer, size_t size) noexcept;
      void  write(pa_t pa, const void* buffer, size_t size) noexcept;

    private:
      void  read_write(pa_t pa, void* buffer, size_t size, bool write) noexcept;

      void* va_;
      pe_t* pte_;
  };
}
