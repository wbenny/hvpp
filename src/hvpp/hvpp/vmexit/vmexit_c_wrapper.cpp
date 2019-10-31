#include "vmexit_c_wrapper.h"

#include "hvpp/vcpu.h"

namespace hvpp {

vmexit_c_wrapper_handler::vmexit_c_wrapper_handler(
  const c_handler_array_t& c_handlers,
  c_handler_setup_fn_t setup_callback,
  c_handler_teardown_fn_t teardown_callback,
  c_handler_terminate_fn_t terminate_callback,
  void* context /*= nullptr */
  ) noexcept
{
  //
  // Make local copy of the C-handlers.
  //

  c_handlers_ = c_handlers;
  setup_callback_ = setup_callback;
  teardown_callback_ = teardown_callback;
  terminate_callback_ = terminate_callback;
  context_ = context;
}

vmexit_c_wrapper_handler::~vmexit_c_wrapper_handler() noexcept
{

}

auto vmexit_c_wrapper_handler::setup(vcpu_t& vp) noexcept -> error_code_t
{
  if (setup_callback_)
  {
    //
    // C-handler has been defined - call that routine.
    //

    auto context = passthrough_context{
      .passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough_setup),
      .context             = context_,
      .handler_instance    = this,
      .handler_method      = nullptr,
      .vcpu                = &vp,
    };

    return (error_code_t)setup_callback_(&vp, &context);
  }
  else
  {
    //
    // C-handler has not been defined - call the pass-through handler.
    //

    return base_type::setup(vp);
  }
}

void vmexit_c_wrapper_handler::teardown(vcpu_t& vp) noexcept
{
  if (teardown_callback_)
  {
    //
    // C-handler has been defined - call that routine.
    //

    auto context = passthrough_context{
      .passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough_teardown),
      .context             = context_,
      .handler_instance    = this,
      .handler_method      = nullptr,
      .vcpu                = &vp,
    };

    teardown_callback_(&vp, &context);
  }
  else
  {
    //
    // C-handler has not been defined - call the pass-through handler.
    //

    base_type::teardown(vp);
  }
}

void vmexit_c_wrapper_handler::terminate(vcpu_t& vp) noexcept
{
  if (terminate_callback_)
  {
    //
    // C-handler has been defined - call that routine.
    //

    auto context = passthrough_context{
      .passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough_terminate),
      .context             = context_,
      .handler_instance    = this,
      .handler_method      = nullptr,
      .vcpu                = &vp,
    };

    terminate_callback_(&vp, &context);
  }
  else
  {
    //
    // C-handler has not been defined - call the pass-through handler.
    //

    base_type::terminate(vp);
  }
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

    auto context = passthrough_context{
      .passthrough_routine = passthrough_fn_t(&vmexit_c_wrapper_handler::handle_passthrough_handler),
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

auto vmexit_c_wrapper_handler::handle_passthrough_setup(passthrough_context* context) noexcept -> error_code_t
{
  const auto  handler_instance =  context->handler_instance;
        auto& vp               = *context->vcpu;

  return handler_instance->base_type::setup(vp);
}

void vmexit_c_wrapper_handler::handle_passthrough_teardown(passthrough_context* context) noexcept
{
  const auto  handler_instance =  context->handler_instance;
        auto& vp               = *context->vcpu;

  handler_instance->base_type::teardown(vp);
}

void vmexit_c_wrapper_handler::handle_passthrough_terminate(passthrough_context* context) noexcept
{
  const auto  handler_instance =  context->handler_instance;
        auto& vp               = *context->vcpu;

  handler_instance->base_type::terminate(vp);
}

void vmexit_c_wrapper_handler::handle_passthrough_handler(passthrough_context* context) noexcept
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
