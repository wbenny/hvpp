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
    virtual ~device() noexcept { destroy(); }

    virtual const char* name() const noexcept = 0;

    auto create() noexcept -> error_code_t;
    void destroy() noexcept;

    //
    // Dispatch methods.
    //

    //
    // A function declared with a return type that uses a placeholder
    // type shall not be virtual ([class.virtual]).
    // (ref: C++ standard, [dcl.spec.auto])
    //
    // TL;DR:
    //   We can't use "virtual auto on_create() -> error_code_t" :(
    //

    virtual error_code_t on_create() noexcept
    { return {}; }

    virtual error_code_t on_close() noexcept
    { return {}; }

    virtual error_code_t on_read(void* buffer, size_t buffer_size, size_t& bytes_read) noexcept
    { (void)(buffer); (void)(buffer_size); (void)(bytes_read); return {}; }

    virtual error_code_t on_write(void* buffer, size_t buffer_size, size_t& bytes_written) noexcept
    { (void)(buffer); (void)(buffer_size); (void)(bytes_written); return {}; }

    virtual error_code_t on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept
    { (void)(buffer); (void)(buffer_size); (void)(code); return {}; }

    static error_code_t copy_from_user(void* buffer_to, const void* buffer_from, size_t length) noexcept;
    static error_code_t copy_to_user(void* buffer_to, const void* buffer_from, size_t length) noexcept;

  private:
    void* impl_;
};
