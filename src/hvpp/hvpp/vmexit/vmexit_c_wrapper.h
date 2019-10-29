#pragma once
#include "vmexit_passthrough.h"

#include <array>

namespace hvpp {

class vmexit_c_wrapper_handler
  : public vmexit_passthrough_handler
{
  public:
    using base_type = vmexit_passthrough_handler;

    using c_handler_fn_t = void(*)(
      void* /* vcpu_t* */,
      void* /* passthrough_context* */
      );

    using c_handler_setup_fn_t = long /* NTSTATUS */(*)(
      void* /* vcpu_t* */,
      void* /* passthrough_context* */
      );

    using c_handler_teardown_fn_t = void(*)(
      void* /* vcpu_t* */,
      void* /* passthrough_context* */
      );

    using c_handler_array_t = std::array<c_handler_fn_t, 65>;

    vmexit_c_wrapper_handler(const c_handler_array_t& c_handlers, c_handler_setup_fn_t setup_callback = nullptr, c_handler_teardown_fn_t teardown_callback = nullptr, void* context = nullptr) noexcept;
    ~vmexit_c_wrapper_handler() noexcept override;

    auto setup(vcpu_t& vp) noexcept -> error_code_t override;
    void teardown(vcpu_t& vp) noexcept override;

    void handle(vcpu_t& vp) noexcept override;

  private:
    using passthrough_fn_t = void(*)(void*);

    template <
      typename THandlerMethod
    >
    struct context_base
    {
      passthrough_fn_t          passthrough_routine;
      void*                     context;
      vmexit_c_wrapper_handler* handler_instance;
      THandlerMethod            handler_method;
      vcpu_t*                   vcpu;
    };

    using handler_method_t    = handler_fn_t;
    using setup_method_t      = error_code_t(base_type::*)(vcpu_t&);
    using teardown_method_t   = void (vmexit_c_wrapper_handler::*)(vcpu_t&);

    using passthrough_handler_context   = context_base<handler_method_t>;
    using passthrough_setup_context     = context_base<setup_method_t>;
    using passthrough_teardown_context  = context_base<teardown_method_t>;

    static auto handle_passthrough_setup(passthrough_setup_context* context) noexcept -> error_code_t;
    static void handle_passthrough_teardown(passthrough_teardown_context* context) noexcept;
    static void handle_passthrough_handler(passthrough_handler_context* context) noexcept;

    c_handler_array_t c_handlers_;
    c_handler_setup_fn_t setup_callback_;
    c_handler_teardown_fn_t teardown_callback_;
    void* context_;
};

}
