#pragma once
#include <hvpp/lib/device.h>
#include <hvpp/vmexit/vmexit_dbgbreak.h>

#include <cstdint>

using ioctl_enable_io_debugbreak_t = ioctl_read_write_t<1, sizeof(uint16_t)>;

class device_custom
  : public device
{
  public:
    const char* name() const noexcept override { return "hvpp"; }

    auto handler() noexcept -> hvpp::vmexit_dbgbreak_handler&;
    void handler(hvpp::vmexit_dbgbreak_handler& handler_instance) noexcept;

    error_code_t on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept override;

  private:
    error_code_t ioctl_enable_io_debugbreak(void* buffer, size_t buffer_size);

    hvpp::vmexit_dbgbreak_handler* handler_ = nullptr;
};
