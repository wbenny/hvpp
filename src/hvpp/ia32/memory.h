#pragma once
#include "win32/memory.h"
#include "lib/log.h"

#include <cstdint>
#include <numeric>

namespace ia32 {

using pfn_t = uint64_t;
using la_t = uint64_t;

enum class memory_type : uint8_t
{
  uncacheable     = 0,
  write_combining = 1,
  write_through   = 4,
  write_protected = 5,
  write_back      = 6,
  invalid         = 0xff
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

static constexpr auto page_shift = 12;
static constexpr auto page_size  = 4096;

template <typename T, typename = std::enable_if_t<std::is_pointer_v<T> || (std::is_integral_v<T> && sizeof(T) == sizeof(uintptr_t))>>
inline constexpr void* page_align(T va) noexcept
{ return (void*)((uintptr_t)(va) & ~(page_size - 1)); }

template <typename T, typename = std::enable_if_t<std::is_pointer_v<T> || (std::is_integral_v<T> && sizeof(T) == sizeof(uintptr_t))>>
inline constexpr uint32_t byte_offset(T va) noexcept
{ return (uint32_t)((uintptr_t)(va) & (page_size - 1)); }

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline constexpr uint64_t bytes_to_pages(T size) noexcept
{ return (size >> page_shift) + ((size & (page_size - 1)) != 0); }

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline constexpr uint64_t round_to_pages(T size) noexcept
{ return ((uintptr_t)(size) + page_size - 1) & ~(page_size - 1); }

class pa_t
{
  public:
    //
    // Static
    //
    static pa_t from_pfn(pfn_t pfn)    noexcept { return pa_t(pfn << page_shift);       }
    static pa_t from_va(void* va)      noexcept { return pa_t(detail::pa_from_va(va));  }

    //
    // ctor/operators
    //

    pa_t()                             noexcept = default;
    pa_t(const pa_t& other)            noexcept = default;
    pa_t(pa_t&& other)                 noexcept = default;
    pa_t& operator=(const pa_t& other) noexcept = default;
    pa_t& operator=(pa_t&& other)      noexcept = default;
    pa_t(uint64_t pa)                  noexcept : value_(pa)   {                        }

    pa_t& operator= (uint64_t other)   noexcept { value_ = other; return *this;         }

    pa_t  operator+ (pa_t other)       noexcept { return pa_t(value_ + other.value_);   }
    pa_t& operator+=(pa_t other)       noexcept { value_ += other.value_; return *this; }

    pa_t  operator- (pa_t other)       noexcept { return pa_t(value_ - other.value_);   }
    pa_t& operator-=(pa_t other)       noexcept { value_ -= other.value_; return *this; }

    bool  operator> (pa_t other) const noexcept { return value_ > other.value_;         }
    bool  operator>=(pa_t other) const noexcept { return value_ >= other.value_;        }
    bool  operator< (pa_t other) const noexcept { return value_ < other.value_;         }
    bool  operator<=(pa_t other) const noexcept { return value_ <= other.value_;        }
    bool  operator==(pa_t other) const noexcept { return value_ == other.value_;        }
    bool  operator!=(pa_t other) const noexcept { return value_ != other.value_;        }
    bool  operator! (          ) const noexcept { return !value_;                       }

    //
    // Getters
    //

    uint64_t value()             const noexcept { return value_;                        }
    pfn_t pfn()                  const noexcept { return value_ >> page_shift;          }
    void* va()                   const noexcept { return detail::va_from_pa(value_);    }

    int index(page_table_level level) const noexcept
    {
      uint64_t result = value_;
      result >>= page_shift + static_cast<uint8_t>(level) * 9;
      result  &= (1 << 9) - 1; // 0x1ff

      return static_cast<int>(result);
    }

  private:
    uint64_t value_;
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

    pa_t operator++(   )       noexcept { return                value_ += page_size;                    }
    pa_t operator++(int)       noexcept { pa_t result = value_; value_ += page_size; return result;     }
    pa_t operator--(   )       noexcept { return                value_ -= page_size;                    }
    pa_t operator--(int)       noexcept { pa_t result = value_; value_ -= page_size; return result;     }
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
    memory_range() noexcept = default;
    memory_range(const memory_range& other) noexcept = default;
    memory_range(memory_range&& other) noexcept = default;
    memory_range& operator=(const memory_range& other) noexcept = default;
    memory_range& operator=(memory_range&& other) noexcept = default;
    memory_range(pa_t begin_pa, pa_t end_pa) noexcept
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
    size_t        size()  const noexcept { return end_.value() - begin_.value(); }

  private:
    pa_t begin_;
    pa_t end_;
};

//
// Class for receiving physical memory ranges which are backed up by actual
// physical memory.
//
class physical_memory_descriptor
{
  public:
    static constexpr int max_range_count = 32;

    physical_memory_descriptor() noexcept { check_physical_memory(); }
    physical_memory_descriptor(const physical_memory_descriptor& other) noexcept = delete;
    physical_memory_descriptor(physical_memory_descriptor&& other) noexcept = delete;
    physical_memory_descriptor& operator=(const physical_memory_descriptor& other) noexcept = delete;
    physical_memory_descriptor& operator=(physical_memory_descriptor&& other) noexcept = delete;

    const memory_range* begin() const noexcept { return &range_[0]; }
    const memory_range* end()   const noexcept { return &range_[size()]; }
    size_t              size()  const noexcept { return count_; }

    size_t total_physical_memory_size() const noexcept
    {
      return std::accumulate(begin(), end(), size_t(0), [](auto sum, auto next) {
        return sum + next.size();
      });
    }

    void dump() const noexcept
    {
      hvpp_info("Physical memory ranges (%i)", count_);

      for (int i = 0; i < count_; ++i)
      {
        hvpp_info(
          "  %2i) [%p - %p] (%8u kb)", i,
          range_[i].begin(),
          range_[i].end(),
          range_[i].size() / 1024);
      }
    }

  private:
    void check_physical_memory() noexcept;

    memory_range range_[max_range_count];
    int          count_ = 0;
};

inline constexpr const char* memory_type_to_string(memory_type type) noexcept
{
  switch (type)
  {
    case memory_type::uncacheable: return "UC";
    case memory_type::write_combining: return "WC";
    case memory_type::write_through: return "WT";
    case memory_type::write_protected: return "WP";
    case memory_type::write_back: return "WB";
    default: return "";
  }
};

}
