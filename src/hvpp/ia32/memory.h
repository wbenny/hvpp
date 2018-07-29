#pragma once
#include "lib/enumname.h"
#include "lib/log.h"

#include <cstdint>
#include <ntddk.h>

namespace ia32 {

using pfn_t = uint64_t;
using la_t = uint64_t;

enum class mtype : uint8_t
{
  uncacheable     = 0,
  write_combining = 1,
  write_through   = 4,
  write_protected = 5,
  write_back      = 6,
  invalid         = 0xff
};

enum class page_size : uint32_t
{
  _4kb = 4 * 1024,
  _2mb = 2 * 1024 * 1024,
  _1gb = 1 * 1024 * 1024 * 1024
};

enum class page_table_level : uint8_t
{
  pml4 = 3,
  pdpt = 2,
  pd   = 1,
  pt   = 0,
};

inline page_table_level& operator++(page_table_level& ptl) noexcept
{ reinterpret_cast<uint8_t&>(ptl)++; return ptl; }

inline page_table_level& operator--(page_table_level& ptl) noexcept
{ reinterpret_cast<uint8_t&>(ptl)--; return ptl; }

inline page_table_level operator++(page_table_level& ptl, int) noexcept
{ auto result = ptl; reinterpret_cast<uint8_t&>(ptl)++; return result; }

inline page_table_level operator--(page_table_level& ptl, int) noexcept
{ auto result = ptl; reinterpret_cast<uint8_t&>(ptl)--; return result; }

inline page_table_level operator+(page_table_level ptl, uint8_t value) noexcept
{ return static_cast<page_table_level>(static_cast<uint8_t>(ptl) + value); }

inline page_table_level operator-(page_table_level ptl, uint8_t value) noexcept
{ return static_cast<page_table_level>(static_cast<uint8_t>(ptl) - value); }

inline page_table_level& operator+=(page_table_level& ptl, uint8_t value) noexcept
{ reinterpret_cast<uint8_t&>(ptl) += value; return ptl; }

inline page_table_level& operator-=(page_table_level& ptl, uint8_t value) noexcept
{ reinterpret_cast<uint8_t&>(ptl) -= value; return ptl; }

class pa_t
{
  public:
    //
    // Static
    //
    static pa_t from_pfn(pfn_t pfn)    noexcept { return pa_t(pfn << PAGE_SHIFT);                 }
    static pa_t from_va(void* va)      noexcept { return pa_t(MmGetPhysicalAddress(va).QuadPart); }

    //
    // ctor/operators
    //

    pa_t()                             noexcept = default;
    pa_t(const pa_t& other)            noexcept = default;
    pa_t(pa_t&& other)                 noexcept = default;
    pa_t& operator=(const pa_t& other) noexcept = default;
    pa_t& operator=(pa_t&& other)      noexcept = default;
    pa_t(uint64_t pa)                  noexcept : address_(pa)   {                               }

    pa_t& operator= (uint64_t other)   noexcept { address_ = other; return *this;                }

    pa_t  operator+ (pa_t other)       noexcept { return pa_t(address_ + other.address_);        }
    pa_t& operator+=(pa_t other)       noexcept { address_ += other.address_; return *this;      }

    pa_t  operator- (pa_t other)       noexcept { return pa_t(address_ - other.address_);        }
    pa_t& operator-=(pa_t other)       noexcept { address_ -= other.address_; return *this;      }

    bool  operator> (pa_t other) const noexcept { return address_ > other.address_;              }
    bool  operator>=(pa_t other) const noexcept { return address_ >= other.address_;             }
    bool  operator< (pa_t other) const noexcept { return address_ < other.address_;              }
    bool  operator<=(pa_t other) const noexcept { return address_ <= other.address_;             }
    bool  operator==(pa_t other) const noexcept { return address_ == other.address_;             }
    bool  operator!=(pa_t other) const noexcept { return address_ != other.address_;             }
    bool  operator! (          ) const noexcept { return !address_;                              }

    //
    // Getters
    //

    uint64_t value()             const noexcept { return address_;                               }
    pfn_t pfn()                  const noexcept { return address_ >> PAGE_SHIFT;                 }
    void* va()                   const noexcept { return MmGetVirtualForPhysical(win_address_);  }

    int index(page_table_level level) const noexcept
    {
      uint64_t result = address_;
      result >>= PAGE_SHIFT + static_cast<uint8_t>(level) * 9;
      result  &= (1 << 9) - 1; // 0x1ff

      return static_cast<int>(result);
    }

  private:
    union
    {
      uint64_t address_;
      PHYSICAL_ADDRESS win_address_;
    };
};

class page_iterator
{
  public:
    page_iterator()                                      noexcept = delete;
    page_iterator(pa_t value)                            noexcept : value_(value) { }
    page_iterator(const page_iterator& other)            noexcept = default;
    page_iterator(page_iterator&& other)                 noexcept = default;
    page_iterator& operator=(const page_iterator& other) noexcept = default;
    page_iterator& operator=(page_iterator&& other)      noexcept = default;

    pa_t operator++(   )       noexcept { return                value_ += PAGE_SIZE;                    }
    pa_t operator++(int)       noexcept { pa_t result = value_; value_ += PAGE_SIZE; return result;     }
    pa_t operator--(   )       noexcept { return                value_ -= PAGE_SIZE;                    }
    pa_t operator--(int)       noexcept { pa_t result = value_; value_ -= PAGE_SIZE; return result;     }
    pa_t operator* (   )       noexcept { return                value_;                                 }
    pa_t operator* (   ) const noexcept { return                value_;                                 }
    pa_t operator->(   )       noexcept { return                value_;                                 }
    pa_t operator->(   ) const noexcept { return                value_;                                 }

    bool operator> (const page_iterator& other) const noexcept { return value_ > other.value_;  }
    bool operator>=(const page_iterator& other) const noexcept { return value_ >= other.value_; }
    bool operator< (const page_iterator& other) const noexcept { return value_ < other.value_;  }
    bool operator<=(const page_iterator& other) const noexcept { return value_ <= other.value_; }
    bool operator==(const page_iterator& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const page_iterator& other) const noexcept { return value_ != other.value_; }
    bool operator! (                                  ) const noexcept { return !value_;                }

  private:
    pa_t value_;
};

class memory_range
{
  public:
    memory_range()                                     noexcept = default;
    memory_range(const memory_range& other)            noexcept = default;
    memory_range(memory_range&& other)                 noexcept = default;
    memory_range& operator=(const memory_range& other) noexcept = default;
    memory_range& operator=(memory_range&& other)      noexcept = default;
    memory_range(pa_t begin_pa, pa_t end_pa)           noexcept
      : begin_(begin_pa)
      , end_(end_pa)
    {

    }

    void set(pa_t begin_pa, pa_t end_pa) noexcept
    {
      begin_ = begin_pa;
      end_ = end_pa;
    }

    bool contains(pa_t pa) const noexcept
    {
      return pa >= begin_ && pa < end_;
    }

    page_iterator begin() const noexcept { return page_iterator(begin_); }
    page_iterator end()   const noexcept { return page_iterator(end_); }

  private:
    pa_t begin_;
    pa_t end_;
};

class physical_memory_descriptor
{
  public:
    void initialize() noexcept
    {
      check_physical_memory();
    }

    void destroy() noexcept
    {

    }

    const memory_range* begin() const noexcept { return &range_[0]; }
    const memory_range* end()   const noexcept { return &range_[count_]; }

  private:
    void check_physical_memory() noexcept
    {
      auto physical_memory_ranges = MmGetPhysicalMemoryRanges();

      do
      {
        pa_t   address = physical_memory_ranges[count_].BaseAddress.QuadPart;
        size_t size    = physical_memory_ranges[count_].NumberOfBytes.QuadPart;

        if (!address && !size)
        {
          break;
        }

        range_[count_] = memory_range(address, address + size);
      } while (++count_ < 32);
    }

    memory_range range_[32];
    int          count_ = 0;
};

}
