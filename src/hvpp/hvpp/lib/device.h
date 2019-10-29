#pragma once
#include "ioctl.h"

#include "error.h"

#include <cstdint>

//
// Simple abstract class for creating devices.
//

class device
{
  public:
    device() noexcept = default;
    device(const device& other) noexcept = delete;
    device(device&& other) noexcept = delete;
    device& operator=(const device& other) noexcept = delete;
    device& operator=(device&& other) noexcept = delete;
    virtual ~device() noexcept
    { destroy(); }

    auto create() noexcept -> error_code_t;
    void destroy() noexcept;

    virtual auto name() const noexcept -> const char * = 0;

    //
    // Dispatch methods.
    //

    virtual auto on_create() noexcept -> error_code_t
    { return {}; }

    virtual auto on_close() noexcept -> error_code_t
    { return {}; }

    virtual auto on_read(void* buffer, size_t buffer_size, size_t& bytes_read) noexcept -> error_code_t
    { (void)(buffer); (void)(buffer_size); (void)(bytes_read); return {}; }

    virtual auto on_write(void* buffer, size_t buffer_size, size_t& bytes_written) noexcept -> error_code_t
    { (void)(buffer); (void)(buffer_size); (void)(bytes_written); return {}; }

    virtual auto on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept -> error_code_t
    { (void)(buffer); (void)(buffer_size); (void)(code); return {}; }

    static auto copy_from_user(void* buffer_to, const void* buffer_from, size_t length) noexcept -> error_code_t;
    static auto copy_to_user(void* buffer_to, const void* buffer_from, size_t length) noexcept -> error_code_t;

  private:
    void* impl_ = nullptr;
};
