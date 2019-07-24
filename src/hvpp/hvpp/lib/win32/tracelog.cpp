#define _NO_CRT_STDIO_INLINE

#include "../log.h"

#include <ntddk.h>
#include <TraceLoggingProvider.h>

//
// GUID:
//   {916fcd3e-673b-4610-aaba-0b71e28acd40}
//

TRACELOGGING_DECLARE_PROVIDER(provider);
TRACELOGGING_DEFINE_PROVIDER(
  provider, "HvppProvider",
  (0x916fcd3e, 0x673b, 0x4610, 0xaa, 0xba, 0x0b, 0x71, 0xe2, 0x8a, 0xcd, 0x40)
);

EXTERN_C
NTKERNELAPI
PCHAR
NTAPI
PsGetProcessImageFileName (
    _In_ PEPROCESS Process
    );

namespace logger::detail
{
  void do_print_trace(const char* process_name, const char* function, const char* message) noexcept
  {
    if (test_options(options_t::print_function_name))
    {
      TraceLoggingWrite(provider,
        "MessageEvent",
        TraceLoggingValue(process_name, "ProcessName"),
        TraceLoggingValue(function, "Function"),
        TraceLoggingValue(message, "Message"));
    }
    else
    {
      TraceLoggingWrite(provider,
        "MessageEvent",
        TraceLoggingValue(process_name, "ProcessName"),
        TraceLoggingValue(message, "Message"));
    }
  }

  auto initialize() noexcept -> error_code_t
  {
    if (!NT_SUCCESS(TraceLoggingRegister(provider)))
    {
      return make_error_code_t(std::errc::not_enough_memory);
    }

    return {};
  }

  void destroy() noexcept
  {
    TraceLoggingUnregister(provider);
  }

  void vprint_trace(level_t level, const char* function, const char* format, va_list args) noexcept
  {
    if (test_level(level))
    {
      auto process_name = PsGetProcessImageFileName(PsGetCurrentProcess());
      auto function_name = function;

      char log_message[512];
      vsprintf_s(log_message, std::size(log_message), format, args);

      do_print_trace(process_name, function_name, log_message);
    }
  }
}
