#pragma once
#include <array>
#include <type_traits>

//
// Object class to create type-erasure'd static objects.
// Because there is no CRT in Windows Drivers, we can't just instantiate
// global and/or static variables with non-trivial constructors (which
// e.g. spinlock isn't).
//
// If we took the pretty path and used e.g.:
//   spinlock lock;
//
// The linker would complain with:
//   warning LNK4210: .CRT section exists; there may be unhandled
//   static initializers or terminators
//
// If we want to insist on NOT being allocated on the heap, we can wrap
// the type by the object_t class.  You should not forget to call initialize()
// method before the first use of the object and destroy() method after
// the object will be no longer used.
//
// The instance of the object_t type acts like a regular smart pointer -
// that means the object it wraps is accessible via operator->() and
// operator*().
//

template <typename T>
class object_t
{
  public:
    object_t() noexcept = default;
    object_t(const object_t& other) noexcept = delete;
    object_t(object_t&& other) noexcept = delete;
    object_t& operator=(const object_t& other) noexcept = delete;
    object_t& operator=(object_t&& other) noexcept = delete;

    template <typename ...ARGS>
    void initialize(ARGS&&... args) noexcept
    {
      new (&as_object()) T(std::forward<ARGS>(args)...);
    }

    void destroy() noexcept
    {
      as_object().~T();
    }

    const T* operator->() const noexcept { return &as_object(); }
          T* operator->()       noexcept { return &as_object(); }

    const T& operator*()  const noexcept { return as_object();  }
          T& operator*()        noexcept { return as_object();  }

  private:
    const T& as_object()  const noexcept { return reinterpret_cast<T&>(*object_data_.data()); }
          T& as_object()        noexcept { return reinterpret_cast<T&>(*object_data_.data()); }

    alignas(T)
    std::array<std::byte, sizeof(T)> object_data_;
};
