#pragma once
#include "paging.h"
#include "arch.h"

#include <cstddef>      // std::byte
#include <cstdint>
#include <cinttypes>
#include <numeric>
#include <type_traits>

namespace ia32 {

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
class memory_range;
class physical_memory_range;

namespace detail
{
  uint64_t pa_from_va(const void* va) noexcept;
  void*    va_from_pa(uint64_t pa) noexcept;
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

    static pa_t from_pfn(uint64_t pfn)  noexcept { return pa_t(pfn << page_shift);       }
    static pa_t from_va(const void* va) noexcept { return pa_t(detail::pa_from_va(va));  }

    //
    // ctor/operators
    //

    pa_t()                              noexcept = default;
    pa_t(const pa_t& other)             noexcept = default;
    pa_t(pa_t&& other)                  noexcept = default;
    pa_t& operator=(const pa_t& other)  noexcept = default;
    pa_t& operator=(pa_t&& other)       noexcept = default;
    pa_t(uint64_t pa)                   noexcept : value_{ pa } {                        }

    pa_t& operator= (uint64_t other)    noexcept { value_ = other; return *this;         }

    pa_t  operator+ (pa_t other)  const noexcept { return pa_t(value_ + other.value_);   }
    pa_t& operator+=(pa_t other)        noexcept { value_ += other.value_; return *this; }

    pa_t  operator- (pa_t other)  const noexcept { return pa_t(value_ - other.value_);   }
    pa_t& operator-=(pa_t other)        noexcept { value_ -= other.value_; return *this; }

    pa_t  operator| (pa_t other)  const noexcept { return pa_t(value_ | other.value_);   }
    pa_t& operator|=(pa_t other)        noexcept { value_ |= other.value_; return *this; }

    pa_t  operator& (pa_t other)  const noexcept { return pa_t(value_ & other.value_);   }
    pa_t& operator&=(pa_t other)        noexcept { value_ &= other.value_; return *this; }

    bool  operator< (pa_t other)  const noexcept { return value_  < other.value_;        }
    bool  operator<=(pa_t other)  const noexcept { return value_ <= other.value_;        }
    bool  operator> (pa_t other)  const noexcept { return value_  > other.value_;        }
    bool  operator>=(pa_t other)  const noexcept { return value_ >= other.value_;        }
    bool  operator==(pa_t other)  const noexcept { return value_ == other.value_;        }
    bool  operator!=(pa_t other)  const noexcept { return value_ != other.value_;        }
    bool  operator! (          )  const noexcept { return !value_;                       }

    explicit operator bool()      const noexcept { return value_ != 0;                   }

    //
    // Getters
    //

    auto value()                  const noexcept { return value_;                        }
    auto pfn()                    const noexcept { return value_ >> page_shift;          }
    auto va()                     const noexcept { return detail::va_from_pa(value_);    }

    int offset(pml level) const noexcept
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
    static constexpr auto bits = 48;
    static constexpr auto mask = (1ull << bits) - 1;

    va_t()                              noexcept = default;
    va_t(const va_t& other)             noexcept = default;
    va_t(va_t&& other)                  noexcept = default;
    va_t& operator=(const va_t& other)  noexcept = default;
    va_t& operator=(va_t&& other)       noexcept = default;
    va_t(const void* va)                noexcept : value_{ uint64_t(va) } {              }
    va_t(uint64_t va)                   noexcept : value_{ va } {                        }

    va_t& operator= (uint64_t other)    noexcept { value_ = other; return *this;         }

    va_t  operator+ (va_t other)  const noexcept { return va_t(value_ + other.value_);   }
    va_t& operator+=(va_t other)        noexcept { value_ += other.value_; return *this; }

    va_t  operator- (va_t other)  const noexcept { return va_t(value_ - other.value_);   }
    va_t& operator-=(va_t other)        noexcept { value_ -= other.value_; return *this; }

    va_t  operator| (va_t other)  const noexcept { return va_t(value_ | other.value_);   }
    va_t& operator|=(va_t other)        noexcept { value_ |= other.value_; return *this; }

    va_t  operator& (va_t other)  const noexcept { return va_t(value_ & other.value_);   }
    va_t& operator&=(va_t other)        noexcept { value_ &= other.value_; return *this; }

    bool  operator< (va_t other)  const noexcept { return value_  < other.value_;        }
    bool  operator<=(va_t other)  const noexcept { return value_ <= other.value_;        }
    bool  operator> (va_t other)  const noexcept { return value_  > other.value_;        }
    bool  operator>=(va_t other)  const noexcept { return value_ >= other.value_;        }
    bool  operator==(va_t other)  const noexcept { return value_ == other.value_;        }
    bool  operator!=(va_t other)  const noexcept { return value_ != other.value_;        }
    bool  operator! (          )  const noexcept { return !value_;                       }

    explicit operator bool()      const noexcept { return value_ != 0;                   }

    auto  value()                 const noexcept { return value_;                        }
    auto  ptr()                   const noexcept { return (const void*)(value_);         }

    auto  canonical()             const noexcept { return va_t(value_ & mask);           }
    bool  is_canonical()          const noexcept { return ((value_ >> bits) + 1) <= 1;   }

    uint64_t index(pml level) const noexcept
    {
      uint64_t result = value_;
      result >>= page_shift + static_cast<uint8_t>(level) * 9;

      return result;
    }

    int offset(pml level) const noexcept
    {
      uint64_t result = index(level);
      result &= (1 << 9) - 1; // 0x1ff

      return static_cast<int>(result);
    }

    pe_t* pt_entry(pml level = pml::pt) const noexcept;

  private:
    uint64_t value_;
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
    memory_range(const void* begin_va, const void* end_va) noexcept
      : begin_{ reinterpret_cast<const std::byte*>(begin_va) }
      , end_{ reinterpret_cast<const std::byte*>(end_va) }
    { }

    memory_range(const void* data, size_t size) noexcept
      : begin_{ reinterpret_cast<const std::byte*>(data) }
      , end_{ reinterpret_cast<const std::byte*>(data) + size }
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

    void set(const void* begin_va, const void* end_va) noexcept
    {
      begin_ = reinterpret_cast<const std::byte*>(begin_va);
      end_   = reinterpret_cast<const std::byte*>(end_va);
    }

    void set(const void* data, size_t size) noexcept
    {
      begin_ = reinterpret_cast<const std::byte*>(data);
      end_   = reinterpret_cast<const std::byte*>(data) + size;
    }

    bool contains(const void* va) const noexcept
    {
      return
        uintptr_t(va) >= uintptr_t(begin_) &&
        uintptr_t(va)  < uintptr_t(end_);
    }

    auto begin() const noexcept { return begin_;                           }
    auto end()   const noexcept { return end_;                             }
    auto data()  const noexcept { return static_cast<const void*>(begin_); }
    auto size()  const noexcept { return end_ - begin_;                    }
    bool empty() const noexcept { return size() == 0;                      }

  private:
    const std::byte* begin_;
    const std::byte* end_;
};

class physical_memory_range
{
  public:
    physical_memory_range() noexcept = default;
    physical_memory_range(const physical_memory_range& other) noexcept = default;
    physical_memory_range(physical_memory_range&& other) noexcept = default;
    physical_memory_range(pa_t begin_pa, pa_t end_pa) noexcept
      : begin_{ begin_pa }
      , end_{ end_pa }
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

    auto begin() const noexcept { return begin_; }
    auto end()   const noexcept { return end_; }
    auto size()  const noexcept { return static_cast<size_t>(end_.value() - begin_.value()); }

  private:
    pa_t begin_;
    pa_t end_;
};

constexpr inline const char* to_string(memory_type type) noexcept
{
  switch (type)
  {
    case memory_type::uncacheable: return "UC";
    case memory_type::write_combining: return "WC";
    case memory_type::write_through: return "WT";
    case memory_type::write_protected: return "WP";
    case memory_type::write_back: return "WB";
  }

  return "";
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
