#pragma once
#include "hvpp/ia32/arch.h"

//
// This simple class can be used for effectively switching to virtual address
// space of any user-mode process (and back to hypervisor's CR3, thanks to
// RAII).  You can think of it as a REALLY simple KeStackAttachProcess and
// KeStackDetachProcess routines.
//
// Use this class with very big caution, because any virtual address of any
// user-mode process can be paged out at any time - and causing page-fault
// in VM-exit is super dangerous (because interrupts are disabled and we're
// technically running at HIGH_IRQL; page-faults are forbidden even at
// DISPATCH_LEVEL).
//
// On Windows someone may think that MmIsAddressValid can help (this function
// checks if access to specific virtual address will cause page-fault), but
// since we don't have control over Windows' internal PFN lock, the return
// value of this function might be unreliable.
//
// Note:
//   This class is non-copyable, but movable.
//

namespace detail
{
  ia32::cr3_t kernel_cr3(ia32::cr3_t cr3) noexcept;
}

class cr3_guard
{
  public:
    cr3_guard(ia32::cr3_t new_cr3) noexcept
      : previous_cr3_{ ia32::read<ia32::cr3_t>() }
    { ia32::write<ia32::cr3_t>(::detail::kernel_cr3(new_cr3)); }

    cr3_guard(const cr3_guard& other) noexcept = delete;

    cr3_guard(cr3_guard&& other) noexcept
      : previous_cr3_{ other.previous_cr3_ }
    { other.previous_cr3_.flags = 0; }

    ~cr3_guard() noexcept
    { if (previous_cr3_.flags) ia32::write<ia32::cr3_t>(previous_cr3_); }

    cr3_guard& operator=(const cr3_guard& other) noexcept = delete;

    cr3_guard& operator=(cr3_guard&& other) noexcept
    { previous_cr3_ = other.previous_cr3_;
      other.previous_cr3_.flags = 0; }

  private:
    ia32::cr3_t previous_cr3_;
};
