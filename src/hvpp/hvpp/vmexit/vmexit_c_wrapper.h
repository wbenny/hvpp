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

    using c_handler_array_t = std::array<c_handler_fn_t, 65>;

    vmexit_c_wrapper_handler(const c_handler_array_t& c_handlers, void* context = nullptr) noexcept;
    ~vmexit_c_wrapper_handler() noexcept override;

    void setup(vcpu_t& vp) noexcept override;
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

    static void handle_passthrough(passthrough_context* context) noexcept;

    c_handler_array_t c_handlers_;
    void* context_;
};

}
