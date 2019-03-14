#pragma once
#include "hvpp/lib/log.h"
#include "paging.h"
#include "arch.h"

#include <cstddef>      // std::byte
#include <cstdint>
#include <numeric>
#include <type_traits>

namespace ia32 {

using pfn_t = uint64_t; // Page Frame Number

enum class memory_type : uint8_t
{
  uncacheable     = 0,
  write_combining = 1,
  write_through   = 4,
  write_protected = 5,
  write_back      = 6,
  invalid         = 0xff
};

static constexpr auto page_shift = 12;
static constexpr auto page_size  = 4096;
static constexpr auto page_mask  = page_size - 1;

class pa_t;
class va_t;
class mapping_t;
class physical_memory_range;
class physical_memory_descriptor;

namespace detail
{
  uint64_t pa_from_va(void* va) noexcept;
  uint64_t pa_from_va(void* va, cr3_t cr3) noexcept;
  void*    va_from_pa(uint64_t pa) noexcept;
  void*    mapping_allocate(size_t size) noexcept;
  void     mapping_free(void* va) noexcept;
  void     check_physical_memory(physical_memory_range* range_list, int range_list_size, int& count) noexcept;
}

//
// Physical address
//

class pa_t
{
  public:
    //
    // Static
    //

    static pa_t from_pfn(pfn_t pfn)    noexcept { return pa_t(pfn << page_shift);       }
    static pa_t from_va(void* va)      noexcept { return pa_t(detail::pa_from_va(va));  }
    static pa_t from_va(void* va,
                        cr3_t cr3)     noexcept { return pa_t(detail::pa_from_va(va, cr3)); }

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

    pa_t  operator+ (pa_t other) const noexcept { return pa_t(value_ + other.value_);   }
    pa_t& operator+=(pa_t other)       noexcept { value_ += other.value_; return *this; }

    pa_t  operator- (pa_t other) const noexcept { return pa_t(value_ - other.value_);   }
    pa_t& operator-=(pa_t other)       noexcept { value_ -= other.value_; return *this; }

    pa_t  operator| (pa_t other) const noexcept { return pa_t(value_ | other.value_);   }
    pa_t& operator|=(pa_t other)       noexcept { value_ |= other.value_; return *this; }

    pa_t  operator& (pa_t other) const noexcept { return pa_t(value_ & other.value_);   }
    pa_t& operator&=(pa_t other)       noexcept { value_ &= other.value_; return *this; }

    bool  operator< (pa_t other) const noexcept { return value_  < other.value_;        }
    bool  operator<=(pa_t other) const noexcept { return value_ <= other.value_;        }
    bool  operator> (pa_t other) const noexcept { return value_  > other.value_;        }
    bool  operator>=(pa_t other) const noexcept { return value_ >= other.value_;        }
    bool  operator==(pa_t other) const noexcept { return value_ == other.value_;        }
    bool  operator!=(pa_t other) const noexcept { return value_ != other.value_;        }
    bool  operator! (          ) const noexcept { return !value_;                       }

    explicit operator bool()     const noexcept { return value_ != 0;                   }

    //
    // Getters
    //

    uint64_t value()             const noexcept { return value_;                        }
    pfn_t    pfn()               const noexcept { return value_ >> page_shift;          }
    void*    va()                const noexcept { return detail::va_from_pa(value_);    }

    int index(pml level) const noexcept
    {
      uint64_t result = value_;
      result >>= page_shift + static_cast<uint8_t>(level) * 9;
      result  &= (1 << 9) - 1; // 0x1ff

      return static_cast<int>(result);
    }

  private:
    uint64_t value_;
};

//
// Virtual address
//

class va_t
{
  public:
    va_t()                             noexcept = default;
    va_t(const va_t& other)            noexcept = default;
    va_t(va_t&& other)                 noexcept = default;
    va_t& operator=(const va_t& other) noexcept = default;
    va_t& operator=(va_t&& other)      noexcept = default;
    va_t(void* va)                     noexcept : value_(uint64_t(va))   {              }
    va_t(uint64_t va)                  noexcept : value_(va)   {                        }

    va_t& operator= (uint64_t other)   noexcept { value_ = other; return *this;         }

    va_t  operator+ (va_t other) const noexcept { return va_t(value_ + other.value_);   }
    va_t& operator+=(va_t other)       noexcept { value_ += other.value_; return *this; }

    va_t  operator- (va_t other) const noexcept { return va_t(value_ - other.value_);   }
    va_t& operator-=(va_t other)       noexcept { value_ -= other.value_; return *this; }

    va_t  operator| (va_t other) const noexcept { return va_t(value_ | other.value_);   }
    va_t& operator|=(va_t other)       noexcept { value_ |= other.value_; return *this; }

    va_t  operator& (va_t other) const noexcept { return va_t(value_ & other.value_);   }
    va_t& operator&=(va_t other)       noexcept { value_ &= other.value_; return *this; }

    bool  operator< (va_t other) const noexcept { return value_  < other.value_;        }
    bool  operator<=(va_t other) const noexcept { return value_ <= other.value_;        }
    bool  operator> (va_t other) const noexcept { return value_  > other.value_;        }
    bool  operator>=(va_t other) const noexcept { return value_ >= other.value_;        }
    bool  operator==(va_t other) const noexcept { return value_ == other.value_;        }
    bool  operator!=(va_t other) const noexcept { return value_ != other.value_;        }
    bool  operator! (          ) const noexcept { return !value_;                       }

    uint64_t value()             const noexcept { return value_;                        }
    void*    ptr()               const noexcept { return (void*)(value_);               }

    int index(pml level) const noexcept
    {
      uint64_t result = value_;
      result >>= page_shift + static_cast<uint8_t>(level) * 9;
      result &= (1 << 9) - 1; // 0x1ff

      return static_cast<int>(result);
    }

    pe_t* pt_entry(cr3_t cr3 = read<cr3_t>(), pml level = pml::pt) const noexcept;

  private:
    uint64_t value_;
};

//
// Mapping
//

class mapping_t
{
  public:
    mapping_t() noexcept;
    ~mapping_t() noexcept;

    mapping_t(const mapping_t& other) noexcept = delete;
    mapping_t(mapping_t&& other) noexcept = delete;
    mapping_t& operator=(const mapping_t& other) noexcept = delete;
    mapping_t& operator=(mapping_t&& other) noexcept = delete;

    void* map(pa_t pa) noexcept;
    void  unmap() noexcept;

    void  read(pa_t pa, void* buffer, size_t size) noexcept;
    void  write(pa_t pa, const void* buffer, size_t size) noexcept;

  private:
    void  read_write(pa_t pa, void* buffer, size_t size, bool write) noexcept;

    void* va_;
    pe_t* pte_;
};

//
// Memory range
//

class memory_range
{
  public:
    memory_range() noexcept = default;
    memory_range(const memory_range& other) noexcept = default;
    memory_range(memory_range&& other) noexcept = default;
    memory_range(void* begin_va, void* end_va) noexcept
      : begin_(reinterpret_cast<std::byte*>(begin_va))
      , end_(reinterpret_cast<std::byte*>(end_va))
    { }

    memory_range(void* data, size_t size) noexcept
      : begin_(reinterpret_cast<std::byte*>(data))
      , end_(reinterpret_cast<std::byte*>(data) + size)
    { }

    memory_range& operator=(const memory_range& other) noexcept = default;
    memory_range& operator=(memory_range&& other) noexcept = default;

    bool operator< (const memory_range& rhs) const noexcept
    { return (begin_ < rhs.begin_ ||
       (!(rhs.begin_ < begin_)    &&
              end_   < rhs.end_));                            }

    bool operator<=(const memory_range& rhs) const noexcept
    { return   !(rhs < *this);                                }

    bool operator> (const memory_range& rhs) const noexcept
    { return     rhs < *this;                                 }

    bool operator>=(const memory_range& rhs) const noexcept
    { return !(*this < rhs);                                  }

    bool operator==(const memory_range& rhs) const noexcept
    { return begin_ == rhs.begin_ &&
             end_   == rhs.end_;                              }

    bool operator!=(const memory_range& rhs) const noexcept
    { return !(*this == rhs);                                 }

    void set(void* begin_va, void* end_va) noexcept
    {
      begin_ = reinterpret_cast<std::byte*>(begin_va);
      end_   = reinterpret_cast<std::byte*>(end_va);
    }

    void set(void* data, size_t size) noexcept
    {
      begin_ = reinterpret_cast<std::byte*>(data);
      end_   = reinterpret_cast<std::byte*>(data) + size;
    }

    bool contains(void* va) const noexcept
    {
      return
        uintptr_t(va) >= uintptr_t(begin_) &&
        uintptr_t(va)  < uintptr_t(end_);
    }

    std::byte* begin() const noexcept { return begin_;        }
    std::byte* end()   const noexcept { return end_;          }
    void*      data()  const noexcept { return begin_;        }
    size_t     size()  const noexcept { return end_ - begin_; }
    bool       empty() const noexcept { return size() == 0;   }

  private:
    std::byte* begin_;
    std::byte* end_;
};

class physical_memory_range
{
  public:
    physical_memory_range() noexcept = default;
    physical_memory_range(const physical_memory_range& other) noexcept = default;
    physical_memory_range(physical_memory_range&& other) noexcept = default;
    physical_memory_range(pa_t begin_pa, pa_t end_pa) noexcept
      : begin_(begin_pa)
      , end_(end_pa)
    { }

    physical_memory_range& operator=(const physical_memory_range& other) noexcept = default;
    physical_memory_range& operator=(physical_memory_range&& other) noexcept = default;

    void set(pa_t begin_pa, pa_t end_pa) noexcept
    {
      begin_ = begin_pa;
      end_   = end_pa;
    }

    bool contains(pa_t pa) const noexcept
    {
      return pa >= begin_ && pa < end_;
    }

    pa_t    begin() const noexcept { return begin_; }
    pa_t    end()   const noexcept { return end_; }
    size_t  size()  const noexcept { return end_.value() - begin_.value(); }

  private:
    pa_t begin_;
    pa_t end_;
};

//
// Class for receiving physical memory ranges which are backed up
// by actual physical memory.
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

    const physical_memory_range* begin() const noexcept { return &range_[0]; }
    const physical_memory_range* end()   const noexcept { return &range_[size()]; }
    size_t                       size()  const noexcept { return count_; }

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
          "  %3i)    [%p - %p] (%8u kb)", i,
          range_[i].begin().value(),
          range_[i].end().value(),
          range_[i].size() / 1024);
      }
    }

  private:
    void check_physical_memory() noexcept
    { detail::check_physical_memory(range_, max_range_count, count_); }

    physical_memory_range range_[max_range_count];
    int                   count_ = 0;
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

//
// Make pa_t, va_t and memory_range hashable.
//

namespace std
{
  template <>
  struct hash<ia32::pa_t>
  {
    size_t operator()(const ia32::pa_t& value) const noexcept
    {
      return std::hash<decltype(value.value())>{}(value.value());
    }
  };

  template <>
  struct hash<ia32::va_t>
  {
    size_t operator()(const ia32::va_t& value) const noexcept
    {
      return std::hash<decltype(value.value())>{}(value.value());
    }
  };

  template <>
  struct hash<ia32::memory_range>
  {
    size_t operator()(const ia32::memory_range& value) const noexcept
    {
      //
      // const auto a = uint32_t(value.begin());
      // const auto a = uint32_t(value.begin());
      //
      // return std::hash<uint64_t>{}(a << 32 | b);
      //

      //
      // ref:
      // https://github.com/tensorflow/tensorflow/blob/3c2033678feb046caede91832045ac8bacb2f95a/tensorflow/core/lib/hash/hash.h#L43
      //
      const auto a = uint64_t(value.begin());
      const auto b = uint64_t(value.end());

      return static_cast<size_t>(a ^ (b + 0x9e3779b97f4a7800ULL + (a << 10) + (a >> 4)));
    }
  };
}
