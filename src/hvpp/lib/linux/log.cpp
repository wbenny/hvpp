#include "../log.h"

#include "lib/mp.h"

#include <algorithm> // std::size

#include <cstring>

extern "C"
{
  void logger_detail_do_print(const char* message);
}

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

    memcpy(buffer, level_string, strlen(level_string));
  }

  template <size_t SIZE>
  void make_function_name(char(&buffer)[SIZE], const char* function) noexcept
  {
    if (!test_options(options_t::print_function_name))
    {
      buffer[0] = '\0';
      return;
    }

    snprintf(buffer, SIZE, "%-40s\t", function);
  }

  template <size_t SIZE>
  void make_log_message(char(&buffer)[SIZE], const char* format, va_list args) noexcept
  {
    vsnprintf(buffer, SIZE, format, args);
  }

  void do_print(const char* message) noexcept
  {
    logger_detail_do_print(message);
  }

  void vprint(level_t level, const char* function, const char* format, va_list args) noexcept
  {
    if (test_level(level))
    {
      char buffer[512];

      char level_string[8];
      make_level(level_string, level);

      // char time[32];
      // make_time(time);
      char time[32] = { 0 };

      // char processor_number[16];
      // make_processor_number(processor_number);
      char processor_number[16] = { 0 };

      char function_name[64];
      make_function_name(function_name, function);

      char log_message[512];
      make_log_message(log_message, format, args);

      // auto process_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PsGetProcessId(PsGetCurrentProcess())));
      // auto thread_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(PsGetCurrentThreadId()));
      // auto process_name = PsGetProcessImageFileName(PsGetCurrentProcess());

      auto process_id = 0;
      auto thread_id = 0;
      const char* process_name = "";

      snprintf(buffer, std::size(buffer), "%s%s%s%5u\t%5u\t%-15s\t%s%s\r\n",
        time, level_string, processor_number,
        process_id, thread_id, process_name,
        function_name, log_message);

      do_print(buffer);
    }
  }
}
