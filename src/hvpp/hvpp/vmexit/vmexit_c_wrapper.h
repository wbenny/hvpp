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

    using c_handler_terminate_fn_t = void(*)(
      void* /* vcpu_t* */,
      void* /* passthrough_context* */
      );

    using c_handler_array_t = std::array<c_handler_fn_t, 65>;

    vmexit_c_wrapper_handler(
      const c_handler_array_t& c_handlers,
      c_handler_setup_fn_t setup_callback = nullptr,
      c_handler_teardown_fn_t teardown_callback = nullptr,
      c_handler_terminate_fn_t terminate_callback = nullptr,
      void* context = nullptr
      ) noexcept;
    ~vmexit_c_wrapper_handler() noexcept override;

    auto setup(vcpu_t& vp) noexcept -> error_code_t override;
    void teardown(vcpu_t& vp) noexcept override;
    void terminate(vcpu_t& vp) noexcept override;

    void handle(vcpu_t& vp) noexcept override;

  private:
    using passthrough_fn_t = void(*)(void*);

    struct passthrough_context
    {
      passthrough_fn_t          passthrough_routine;
      void*                     context;
      vmexit_c_wrapper_handler* handler_instance;
      handler_fn_t              handler_method;
      vcpu_t*                   vcpu;
    };

    static auto handle_passthrough_setup(passthrough_context* context) noexcept -> error_code_t;
    static void handle_passthrough_teardown(passthrough_context* context) noexcept;
    static void handle_passthrough_terminate(passthrough_context* context) noexcept;
    static void handle_passthrough_handler(passthrough_context* context) noexcept;

    c_handler_array_t c_handlers_;
    c_handler_setup_fn_t setup_callback_;
    c_handler_teardown_fn_t teardown_callback_;
    c_handler_terminate_fn_t terminate_callback_;
    void* context_;
};

}
