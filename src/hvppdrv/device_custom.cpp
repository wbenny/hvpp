#include "device_custom.h"

#include <hvpp/lib/assert.h>
#include <hvpp/lib/debugger.h>
#include <hvpp/lib/log.h>

auto device_custom::handler() noexcept -> hvpp::vmexit_dbgbreak_handler&
{
  return *handler_;
}

void device_custom::handler(hvpp::vmexit_dbgbreak_handler& handler_instance) noexcept
{
  handler_ = &handler_instance;
}

error_code_t device_custom::on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept
{
  switch (code)
  {
    case ioctl_enable_io_debugbreak_t::code:
      return ioctl_enable_io_debugbreak(buffer, buffer_size);

    default:
      hvpp_assert(0);
      return make_error_code_t(std::errc::invalid_argument);
  }
}

error_code_t device_custom::ioctl_enable_io_debugbreak(void* buffer, size_t buffer_size)
{
  hvpp_assert(handler_);
  hvpp_assert(buffer);
  hvpp_assert(buffer_size >= ioctl_enable_io_debugbreak_t::size);

  if (!buffer || buffer_size < ioctl_enable_io_debugbreak_t::size)
  {
    return make_error_code_t(std::errc::invalid_argument);
  }

  //
  // Capture the I/O port from the buffer.
  //
  uint16_t& io_port = *((uint16_t*)buffer);

  //
  // Do nothing if kernel debugger is not attached.
  //
  if (!debugger::is_enabled())
  {
    //
    // Set value of the output buffer (example).
    //
    io_port = 0xCAFE;
    return {};
  }

  handler_->storage().io_in[io_port] = true;
  handler_->storage().io_out[io_port] = true;

  hvpp_info("ioctl_enable_io_debugbreak: 0x%04x", io_port);

  //
  // Set value of the output buffer (example).
  //
  io_port = 0x1337;

  return {};
}
