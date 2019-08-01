#pragma once
#include "hvpp/ia32/asm.h"

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <limits>

#ifdef min
# undef min
#endif

#ifdef max
# undef max
#endif

class basic_bitmap
{
  public:
    basic_bitmap() noexcept : buffer_{ nullptr }, size_in_bits_{ 0 } { };
    basic_bitmap(const basic_bitmap& other) noexcept = delete;
    basic_bitmap(basic_bitmap&& other) noexcept = default;
    basic_bitmap& operator=(const basic_bitmap& other) = delete;
    basic_bitmap& operator=(basic_bitmap&& other) = default;

    basic_bitmap(void* buffer, int size_in_bits) noexcept
      : buffer_{ reinterpret_cast<word_t*>(buffer) }
      , size_in_bits_{ size_in_bits } { }

    template <typename T, int SIZE>
    basic_bitmap(T(&buffer)[SIZE], int size_in_bits = SIZE * sizeof(T)) noexcept
      : buffer_{ reinterpret_cast<word_t*>(buffer) }
      , size_in_bits_{ size_in_bits } { }

    ~basic_bitmap() noexcept = default;

    const void* buffer() const noexcept;
    void* buffer() noexcept;

    int size_in_bits() const noexcept;
    int size_in_bytes() const noexcept;

    void set() noexcept;
    void clear() noexcept;

    void set(int bit) noexcept;
    void clear(int bit) noexcept;

    //bool test(int bit) const noexcept { return !!(buffer_[word(bit)] & mask(bit)); }
    bool test(int bit) const noexcept;

    void set(int index, int count) noexcept;
    void clear(int index, int count) noexcept;

    int find_first_set() const noexcept;
    int find_first_set(int index, int count) const noexcept;
    int find_first_set(int count) const noexcept;

    int find_first_clear() const noexcept;
    int find_first_clear(int count) const noexcept;
    int find_first_clear(int index, int count) const noexcept;

    bool are_bits_set(int index, int count) const noexcept;
    bool are_bits_clear(int index, int count) const noexcept;

    bool all_set() const noexcept;
    bool all_clear() const noexcept;

  protected:
    using word_t = uint64_t;
    static constexpr word_t bit_count = sizeof(word_t) * 8;

    static constexpr int    offset(int bit) noexcept { return bit % bit_count; }
    static constexpr word_t word  (int bit) noexcept { return bit / bit_count; }
    static constexpr word_t mask  (int bit) noexcept { return word_t(1) << offset(bit); }

  private:
    int get_length_of_set(int index, int count) const noexcept;
    int get_length_of_clear(int index, int count) const noexcept;

    word_t* buffer_;
    int size_in_bits_;
};

template <
  size_t SIZE_IN_BITS = 0
>
class bitmap;

template <>
class bitmap<>
  : public basic_bitmap
{
  public:
    using basic_bitmap::basic_bitmap;
};

template <
  size_t SIZE_IN_BITS
>
class bitmap
  : public bitmap<>
{
  public:
    bitmap() : bitmap<>{ buffer_, SIZE_IN_BITS }, buffer_{} { }
    bitmap(const bitmap& other) noexcept = delete;
    bitmap(bitmap&& other) noexcept = default;
    bitmap& operator=(const bitmap& other) noexcept = delete;
    bitmap& operator=(bitmap&& other) = default;

    ~bitmap() noexcept = default;

  private:
    word_t buffer_[
       word  (SIZE_IN_BITS) +
      (offset(SIZE_IN_BITS) ? 1 : 0)
    ];
};
