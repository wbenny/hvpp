#define _NO_CRT_STDIO_INLINE

#include "../log.h"

#include "../mp.h"

#include <ntddk.h>

EXTERN_C
NTKERNELAPI
PCHAR
NTAPI
PsGetProcessImageFileName (
    _In_ PEPROCESS Process
    );

namespace logger::detail
{
  template <size_t SIZE>
  void make_level(char (&buffer)[SIZE], level_t level) noexcept
  {
    const char* level_string =
      level == level_t::debug ? "DBG\t" :
      level == level_t::info  ? "INF\t" :
      level == level_t::warn  ? "WRN\t" :
      level == level_t::error ? "ERR\t" :
                                "###\t";

    strcpy_s(buffer, level_string);
  }

  template <size_t SIZE>
  void make_time(char(&buffer)[SIZE]) noexcept
  {
    if (!test_options(options_t::print_time))
    {
      buffer[0] = '\0';
      return;
    }

    LARGE_INTEGER system_time;
    KeQuerySystemTime(&system_time);

    LARGE_INTEGER local_time;
    ExSystemTimeToLocalTime(&system_time, &local_time);

    TIME_FIELDS time_fields;
    RtlTimeToTimeFields(&local_time, &time_fields);

    sprintf_s(buffer, "%02hd:%02hd:%02hd.%03hd\t",
              time_fields.Hour, time_fields.Minute,
              time_fields.Second, time_fields.Milliseconds);
  }

  template <size_t SIZE>
  void make_processor_number(char(&buffer)[SIZE]) noexcept
  {
    if (!test_options(options_t::print_processor_number))
    {
      buffer[0] = '\0';
      return;
    }

    sprintf_s(buffer, "#%u\t", mp::cpu_index());
  }

  template <size_t SIZE>
  void make_function_name(char(&buffer)[SIZE], const char* function) noexcept
  {
    if (!test_options(options_t::print_function_name))
    {
      buffer[0] = '\0';
      return;
    }

    sprintf_s(buffer, "%-40s\t", function);
  }

  template <size_t SIZE>
  void make_log_message(char(&buffer)[SIZE], const char* format, va_list args) noexcept
  {
    vsprintf_s(buffer, format, args);
  }

  void do_print(const char* message) noexcept
  {
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%s", message);
  }

  void vprint(level_t level, const char* function, const char* format, va_list args) noexcept
  {
    if (test_level(level))
    {
      char buffer[512];

      char level_string[8];
      make_level(level_string, level);

      char time[32];
      make_time(time);

      char processor_number[16];
      make_processor_number(processor_number);

      char function_name[64];
      make_function_name(function_name, function);

      char log_message[512];
      make_log_message(log_message, format, args);

      auto process_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PsGetProcessId(PsGetCurrentProcess())));
      auto thread_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PsGetCurrentThreadId()));
      auto process_name = PsGetProcessImageFileName(PsGetCurrentProcess());

      sprintf_s(buffer, "%s%s%s%5u\t%5u\t%-15s\t%s%s\r\n",
        time, level_string, processor_number,
        process_id, thread_id, process_name,
        function_name, log_message);

      do_print(buffer);
    }
  }
}
