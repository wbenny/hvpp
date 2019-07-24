#pragma once
#include <cstdint>

enum class ioctl_access : uint32_t
{
  none        = 0,
  read        = 1,
  write       = 2,

  read_write  = read | write
};

constexpr inline auto
make_ioctl_code_windows(
  uint32_t id,
  ioctl_access access,
  uint32_t size
  ) noexcept
{
  //
  // "Size" isn't part of the IOCTL code on Windows.
  //
  (void)(size);

  //
  // Taken from CTL_CODE() macro.
  //
  constexpr auto ctl_code_impl = [](
    uint32_t DeviceType,
    uint32_t Method,
    uint32_t Function,
    uint32_t Access
    ) noexcept -> uint32_t {
      return (DeviceType << 16)
           | (Access     << 14)
           | (Function   <<  2)
           | (Method);
    };

  //
  // DeviceType
  //   Identifies the device type.  This value must match the
  //   value that is set in the DeviceType member of the driver's
  //   DEVICE_OBJECT structure.  (See Specifying Device Types).
  //   Values of less than 0x8000 are reserved for Microsoft.
  //   Values of 0x8000 and higher can be used by vendors.
  //   Note that the vendor-assigned values set the Common bit.
  //
  // FunctionCode
  //   Identifies the function to be performed by the driver.
  //   Values of less than 0x800 are reserved for Microsoft.
  //   Values of 0x800 and higher can be used by vendors.
  //   Note that the vendor-assigned values set the Custom bit.
  //
  // (ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes)
  //

  return ctl_code_impl(0x00000022,         // FILE_DEVICE_UNKNOWN
                       0,                  // METHOD_BUFFERED
                       0x800 | id,
                       uint32_t(access));
}

constexpr inline auto
make_ioctl_code_linux(
  uint32_t id,
  ioctl_access access,
  uint32_t size
  ) noexcept
{
  //
  // Taken from Linux source code (include/uapi/asm-generic/ioctl.h).
  //
  constexpr auto ctl_code_impl = [](
    uint32_t dir,
    uint32_t type,
    uint32_t nr,
    uint32_t size
    ) noexcept -> uint32_t {
      return ((dir)  <<  0)   // IOC_DIRSHIFT
           | ((type) <<  8)   // IOC_TYPESHIFT
           | ((nr)   << 16)   // IOC_NRSHIFT
           | ((size) << 30);  // IOC_SIZESHIFT
    };

  return ctl_code_impl(uint32_t(access), 'H', id, size);
}

constexpr inline auto
make_ioctl_code(
  uint32_t id,
  ioctl_access access,
  uint32_t size
  ) noexcept
{
#ifdef _WIN32
  return make_ioctl_code_windows(id, access, size);
#elif __linux__
  return make_ioctl_code_linux(id, access, size);
#else
#error Unsupported operating system!
#endif
}

template <
  uint32_t Id,
  ioctl_access Access,
  uint32_t Size
>
struct ioctl_t
{
  static constexpr uint32_t code = make_ioctl_code(Id, Access, Size);
  static constexpr uint32_t size = Size;
};

template <uint32_t Id>
using ioctl_none_t = ioctl_t<Id, ioctl_access::none, 0>;

template <uint32_t Id, uint32_t Size>
using ioctl_read_t = ioctl_t<Id, ioctl_access::read, Size>;

template <uint32_t Id, uint32_t Size>
using ioctl_write_t = ioctl_t<Id, ioctl_access::write, Size>;

template <uint32_t Id, uint32_t Size>
using ioctl_read_write_t = ioctl_t<Id, ioctl_access::read_write, Size>;
