#pragma once
#include "hvpp/ia32/asm.h"

#include <cstdint>
#include <atomic>
#include <algorithm>

#ifdef min
# undef min
#endif

//
// Based on my benchmarks, this simple implementation beats other (often
// more complex) spinlock implementations - such as queue spinlocks, ticket
// spinlocks, MCS locks.  The only difference between this implementation
// and completely naive spinlock is the "backoff".
//
// Also, benefit of this implementation is that we can use it with
// STL lock guards, e.g.: std::lock_guard.
//
// Look here for more information:
//   - https://locklessinc.com/articles/locks/
//   - https://github.com/cyfdecyf/spinlock
//

class spinlock
{
  public:
    //
    // Manually fine-tuned.
    //
    static constexpr unsigned max_wait = 65536;

    bool try_lock() noexcept
    {
      return !lock_.test_and_set(std::memory_order_acquire);
    }

    void lock() noexcept
    {
      unsigned wait = 1;

      while (!try_lock())
      {
        for (unsigned i = 0; i < wait; ++i)
        {
          ia32_asm_pause();
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
        // clamp it to the max_wait.
        //
        wait = std::min(wait * 2, max_wait);
      }
    }

    void unlock() noexcept
    {
      lock_.clear(std::memory_order_release);
    }

  private:
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};
