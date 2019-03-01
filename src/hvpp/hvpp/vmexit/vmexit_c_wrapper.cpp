#include "vmexit_c_wrapper.h"

#include "hvpp/vcpu.h"

namespace hvpp {

auto vmexit_c_wrapper_handler::initialize(const c_handler_array_t& c_handlers, void* context) noexcept -> error_code_t
{
  //
  // Make local copy of the C-handlers.
  //

  c_handlers_ = c_handlers;
  context_ = context;

  return error_code_t{};
}

void vmexit_c_wrapper_handler::destroy() noexcept
{

}

void vmexit_c_wrapper_handler::setup(vcpu_t& vp) noexcept
{
  base_type::setup(vp);

  //
  // Enable only 1 EPT table in C-wrapper for now.
  // The option of having multiple EPTs should be considered
  // in the future.
  //
  vp.ept_enable();
  vp.ept().map_identity();
}

void vmexit_c_wrapper_handler::handle(vcpu_t& vp) noexcept
{
  auto exit_reason = vp.exit_reason();
  auto exit_reason_index = static_cast<int>(exit_reason);

  auto cpp_handler = handlers_[exit_reason_index];
  auto c_handler = c_handlers_[exit_reason_index];

  if (c_handler)
  {
    //
    // C-handler has been defined - call that routine.
    //

    passthrough_context context;
    context.passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough);
    context.context             = context_;
    context.handler_instance    = this;
    context.handler_method      = cpp_handler;
    context.vcpu                = &vp;

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
  // from the pass-trough context and call that method.
  //

  auto  handler_instance =  context->handler_instance;
  auto  handler_method   =  context->handler_method;
  auto& vp               = *context->vcpu;

  (handler_instance->*handler_method)(vp);
}

}
