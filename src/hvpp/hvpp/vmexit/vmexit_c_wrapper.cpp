#include "vmexit_c_wrapper.h"

#include "hvpp/vcpu.h"

namespace hvpp {

vmexit_c_wrapper_handler::vmexit_c_wrapper_handler(const c_handler_array_t& c_handlers, void* context) noexcept
{
  //
  // Make local copy of the C-handlers.
  //

  c_handlers_ = c_handlers;
  context_ = context;
}

vmexit_c_wrapper_handler::~vmexit_c_wrapper_handler() noexcept
{

}

auto vmexit_c_wrapper_handler::setup(vcpu_t& vp) noexcept -> error_code_t
{
  base_type::setup(vp);

  //
  // Enable only 1 EPT table in C-wrapper for now.
  // The option of having multiple EPTs should be considered
  // in the future.
  //
  vp.ept_enable();
  vp.ept().map_identity();

  return {};
}

void vmexit_c_wrapper_handler::handle(vcpu_t& vp) noexcept
{
  const auto exit_reason = vp.exit_reason();
  const auto exit_reason_index = static_cast<int>(exit_reason);

  const auto cpp_handler = handlers_[exit_reason_index];
  const auto c_handler = c_handlers_[exit_reason_index];

  if (c_handler)
  {
    //
    // C-handler has been defined - call that routine.
    //

    auto context = passthrough_context {
      .passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough),
      .context             = context_,
      .handler_instance    = this,
      .handler_method      = cpp_handler,
      .vcpu                = &vp,
    };

    c_handler(&vp, &context);
  }
  else
  {
    //
    // C-handler has not been defined - call the pass-through handler.
    //

    (this->*cpp_handler)(vp);
  }
}

void vmexit_c_wrapper_handler::handle_passthrough(passthrough_context* context) noexcept
{
  //
  // Fetch the handler instance, method and vcpu_t reference
  // from the pass-through context and call that method.
  //

  const auto  handler_instance =  context->handler_instance;
  const auto  handler_method   =  context->handler_method;
        auto& vp               = *context->vcpu;

  (handler_instance->*handler_method)(vp);
}

}
