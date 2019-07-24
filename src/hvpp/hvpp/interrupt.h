#pragma once
#include "ia32/exception.h"
#include "ia32/vmx.h"

namespace hvpp {

class interrupt_t final
{
  public:
    //
    // Constructors.
    //
    constexpr
    interrupt_t() noexcept  // TODO: make default-constructor private!
      : info_{}
      , error_code_{}
      , rip_adjust_{}
    { }

    constexpr
    interrupt_t(
      vmx::interrupt_type interrupt_type,
      exception_vector exception_vector,
      int rip_adjust = -1
      ) noexcept
      : interrupt_t{ interrupt_type,
                     exception_vector,
                     exception_error_code_t{},
                     false,
                     rip_adjust }
    { }

    constexpr
    interrupt_t(
      vmx::interrupt_type interrupt_type,
      exception_vector exception_vector,
      exception_error_code_t exception_code,
      int rip_adjust = -1
      ) noexcept
      : interrupt_t{ interrupt_type,
                     exception_vector,
                     exception_code,
                     true,
                     rip_adjust }
    { }

    //
    // Default copy/move constructor.
    // Default copy/move assignment operator.
    //
    constexpr interrupt_t(const interrupt_t& other)             noexcept = default;
    constexpr interrupt_t(interrupt_t&& other)                  noexcept = default;
    constexpr interrupt_t& operator=(const interrupt_t& other)  noexcept = default;
    constexpr interrupt_t& operator=(interrupt_t&& other)       noexcept = default;

    //
    // Getters.
    //
    constexpr auto vector()                               const noexcept { return static_cast<exception_vector>(info_.vector); }
    constexpr auto type()                                 const noexcept { return static_cast<vmx::interrupt_type>(info_.type); }
    constexpr auto error_code_valid()                     const noexcept { return info_.error_code_valid; }
    constexpr auto nmi_unblocking()                       const noexcept { return info_.nmi_unblocking; }
    constexpr auto valid()                                const noexcept { return info_.valid; }
    constexpr auto error_code()                           const noexcept { return error_code_; }
    constexpr auto rip_adjust()                           const noexcept { return rip_adjust_; }

  private:
    friend class vcpu_t;

    constexpr
    interrupt_t(
      vmx::interrupt_type interrupt_type,
      exception_vector exception_vector,
      exception_error_code_t exception_code,
      bool exception_code_valid,
      int rip_adjust
      ) noexcept
      : error_code_{ exception_code }
      , rip_adjust_{ rip_adjust }
    {
      info_.flags = 0;

      info_.vector = static_cast<uint32_t>(exception_vector);
      info_.type   = static_cast<uint32_t>(interrupt_type);
      info_.valid  = true;

      //
      // Final sanitization of the following fields takes place
      // in vcpu::interrupt_inject_force().
      //

      info_.error_code_valid = exception_code_valid;
    }

    vmx::interrupt_info_t  info_;
    exception_error_code_t error_code_;
    int                    rip_adjust_;
};

namespace interrupt
{
    //
    // Predefined interrupt structures.
    // Helpful when injecting events.
    //

    static constexpr auto nmi =
      interrupt_t {
        vmx::interrupt_type::nmi,
        exception_vector::nmi_interrupt
      };

    static constexpr auto debug =
      interrupt_t {
        vmx::interrupt_type::hardware_exception,
        exception_vector::debug
      };

    static constexpr auto invalid_opcode =
      interrupt_t {
        vmx::interrupt_type::hardware_exception,
        exception_vector::invalid_opcode
      };

    static constexpr auto general_protection =
      interrupt_t {
        vmx::interrupt_type::hardware_exception,
        exception_vector::general_protection,
        exception_error_code_t{}
      };
}

}
