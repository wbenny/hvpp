#pragma once
#include "ioctl.h"

#include "lib/error.h"

#include <cstdint>

//
// Simple abstract class for creating devices.
//

class device
{
  public:
    virtual ~device() noexcept {}

    auto initialize() noexcept -> error_code_t;
    void destroy() noexcept;

    virtual const char* name() const noexcept = 0;

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
    { return error_code_t{}; }

    virtual error_code_t on_close() noexcept
    { return error_code_t{}; }

    virtual error_code_t on_read(void* buffer, size_t buffer_size, size_t& bytes_read) noexcept
    { (void)buffer; (void)buffer_size; (void)bytes_read; return error_code_t{}; }

    virtual error_code_t on_write(void* buffer, size_t buffer_size, size_t& bytes_written) noexcept

    { (void)buffer; (void)buffer_size; (void)bytes_written; return error_code_t{}; }

    virtual error_code_t on_ioctl(void* buffer, size_t buffer_size, uint32_t code) noexcept
    { (void)buffer; (void)buffer_size; (void)code; return error_code_t{}; }

  private:
    void* impl_;
};
