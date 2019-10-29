#pragma once
#include "assert.h"   // hvpp_assert

#include <array>      // std::array
#include <memory>     // std::move
#include <cstddef>    // size_t

template <
  typename T,
  size_t Size
>
class fixed_dequeue
{
  public:
    fixed_dequeue() noexcept = default;
    fixed_dequeue(const fixed_dequeue& other) noexcept = delete;
    fixed_dequeue(fixed_dequeue&& other) noexcept = delete;
    fixed_dequeue& operator=(const fixed_dequeue& other) noexcept = delete;
    fixed_dequeue& operator=(fixed_dequeue&& other) noexcept = delete;
    ~fixed_dequeue() noexcept = default;

    void push_front(const T& item) noexcept
    {
      //
      // Overflow check.
      //
      hvpp_assert(count_ < Size);

      first_ = !first_
        ? Size - 1
        : first_ - 1;

      queue_[first_] = item;
      count_ += 1;
    }

    void push_front(T&& item) noexcept
    {
      //
      // Overflow check.
      //
      hvpp_assert(count_ < Size);

      first_ = !first_
        ? Size - 1
        : first_ - 1;

      queue_[first_] = std::move(item);
      count_ += 1;
    }

    void push_back(const T& item) noexcept
    {
      //
      // Overflow check.
      //
      hvpp_assert(count_ < Size);

      auto index = (first_ + count_) % Size;
      queue_[index] = item;
      count_ += 1;
    }

    void push_back(T&& item) noexcept
    {
      //
      // Overflow check.
      //
      hvpp_assert(count_ < Size);

      auto index = (first_ + count_) % Size;
      queue_[index] = std::move(item);
      count_ += 1;
    }

    void pop_front() noexcept
    {
      //
      // Underflow check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      first_ += 1;
      count_ -= 1;

      if (!count_ || first_ == Size)
      {
        first_ = 0;
      }
    }

    void pop_back() noexcept
    {
      //
      // Underflow check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      count_ -= 1;

      if (!count_)
      {
        first_ = 0;
      }
    }

    const T& front() const noexcept
    {
      //
      // Item presence check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      return queue_[first_];
    }

    T& front() noexcept
    {
      //
      // Item presence check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      return queue_[first_];
    }

    const T& back() const noexcept
    {
      //
      // Item presence check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      return queue_[(first_ + count_ - 1) % Size];
    }

    T& back() noexcept
    {
      //
      // Item presence check.
      //
      hvpp_assert(count_ > 0 && count_ <= Size);

      return queue_[(first_ + count_ - 1) % Size];
    }

    size_t size() const noexcept
    {
      return count_;
    }

    size_t capacity() const noexcept
    {
      return Size;
    }

  private:
    std::array<T, Size> queue_;
    size_t first_ = 0;
    size_t count_ = 0;
};
