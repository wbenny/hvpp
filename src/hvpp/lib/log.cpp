#define _NO_CRT_STDIO_INLINE

#include "log.h"
#include "lib/assert.h"
#include "lib/spinlock.h"
#include "lib/mp.h"

#include <algorithm>
#include <mutex>
#include <cstdarg>

#include <ntddk.h>

_When_(Timeout == NULL, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart != 0, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

EXTERN_C
NTKERNELAPI
PCHAR
NTAPI
PsGetProcessImageFileName (
    _In_ PEPROCESS Process
    );

//
// Threaded logger implementation.
//
// Logger is provided the size of a single buffer and allocates that number
// of bytes for 2 buffers (logger is double-buffered).
// At any point, there is one buffer used for writes and second buffer used for
// reads. These buffers are atomically swapped on each flush (only if the current
// buffer isn't empty).
// Logger also stores head and tail of the current buffer:
//   - head always points to the begin of the current buffer (for write)
//   - tail always points to 1 byte after the last written message.
//
// If buffer is full and the logger cannot enqueue another message, it optionally
// puts error message at the end of the buffer - all consequent writes are safely
// ignored.
//

namespace logger
{
  size_t    buffer_size = 0;
  char*     buffer1 = nullptr;
  char*     buffer2 = nullptr;
  char*     buffer_head = nullptr;
  char*     buffer_tail = nullptr;

  level_t   current_level   = level_t::default;
  options_t current_options = options_t::default;

  //
  // Contains number of failed attempts to enqueue the message into the buffer.
  // This value is reset to 0 with each flush.
  //
  size_t    enqueue_fail_count = 0;

  //
  // See mm.cpp - where the same construction is used - for explanation.
  //
  uint32_t  __lock_storage__;
  spinlock& lock = (spinlock&)__lock_storage__;
  static_assert(sizeof(uint32_t) == sizeof(spinlock));

  HANDLE   thread_handle;
  bool     thread_alive = false;
  int      flush_interval_ms = 50;

  namespace detail
  {
    bool test_level(level_t level) noexcept
    {
      return (current_level & level) == level;
    }

    bool test_option(options_t option) noexcept
    {
      return (current_options & option) == option;
    }

    template <size_t SIZE>
    int make_level(char (&buffer)[SIZE], level_t level) noexcept
    {
      //
      // Copy logging level string to the buffer.
      //
      const char* level_string =
        level == level_t::debug ? "DBG\t" :
        level == level_t::info  ? "INF\t" :
        level == level_t::warn  ? "WRN\t" :
        level == level_t::error ? "ERR\t" :
                                  "###\t";

      strcpy_s(buffer, SIZE, level_string);
      return static_cast<int>(strlen(level_string));
    }

    template <size_t SIZE>
    int make_time(char(&buffer)[SIZE]) noexcept
    {
      if (!test_option(options_t::print_time))
      {
        buffer[0] = '\0';
        return 0;
      }

      LARGE_INTEGER system_time;
      KeQuerySystemTime(&system_time);

      LARGE_INTEGER local_time;
      ExSystemTimeToLocalTime(&system_time, &local_time);

      TIME_FIELDS time_fields;
      RtlTimeToTimeFields(&local_time, &time_fields);

      return sprintf_s(buffer, SIZE, "%02hd:%02hd:%02hd.%03hd\t",
                       time_fields.Hour, time_fields.Minute,
                       time_fields.Second, time_fields.Milliseconds);
    }

    template <size_t SIZE>
    int make_processor_number(char(&buffer)[SIZE]) noexcept
    {
      if (!test_option(options_t::print_processor_number))
      {
        buffer[0] = '\0';
        return 0;
      }

      return sprintf_s(buffer, SIZE, "#%lu\t",
                       mp::cpu_index());
    }

    template <size_t SIZE>
    int make_function_name(char(&buffer)[SIZE], const char* function) noexcept
    {
      if (!test_option(options_t::print_function_name) &&
          !test_option(options_t::print_function_full_name))
      {
        buffer[0] = '\0';
        return 0;
      }

      if (test_option(options_t::print_function_name))
      {
        auto base_function = strrchr(function, ':');
        if (base_function != function)
        {
          function = base_function + 1;
        }
      }

      return sprintf_s(buffer, SIZE, "%-40s\t",
                       function);
    }

    template <size_t SIZE>
    int make_log_message(char(&buffer)[SIZE], const char* format, va_list args) noexcept
    {
      return vsprintf_s(buffer, SIZE, format, args);
    }

    template <size_t SIZE>
    int make_enqueue_fail_count_message(char(&buffer)[SIZE]) noexcept
    {
      return sprintf_s(buffer, SIZE, "Failed to enqueue messages (%8Iu)\r\n",
                       enqueue_fail_count);
    }

    template <size_t SIZE>
    int vprint(char(&buffer)[SIZE], level_t level, const char* function, const char* format, va_list args) noexcept
    {
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
      return sprintf_s(buffer, SIZE, "%s%s%s%5u\t%5u\t%-15s\t%s%s\r\n",
                       time, level_string, processor_number,
                       process_id, thread_id, process_name,
                       function_name, log_message);
    }

    void enqueue_message(const char* message, size_t message_length) noexcept
    {
      //
      // Lock the log buffers, so that any other thread can't mess them up.
      //
      std::lock_guard _(lock);

      //
      // Compute unused buffer size.
      //
      auto unused_buffer_size = buffer_size - (buffer_tail - buffer_head) - 1;

      //
      // Don't put the message to the buffer
      //   - if current message can't fit into the buffer or
      //   - if previous enqueue already failed before flushing (we don't care
      //     whether the message would fit or not)
      //
      if (message_length > unused_buffer_size || enqueue_fail_count > 0)
      {
        //
        // There isn't enough space in the buffer for the current message. Increment
        // the fail count and insert error message at the end of the buffer (if the
        // option is enabled). Note that fail count is reset to 0 with each flush.
        //
        enqueue_fail_count += 1;

        if (test_option(options_t::print_error_on_fail))
        {
          char error_message[64];
          int error_message_length = make_enqueue_fail_count_message(error_message);

          //
          // Copy the message at the end of the current buffer. This will override text
          // of the last message (or messages). This will also leave "\0\0" as a very
          // last bytes in the buffer, which is desirable for the flush() loop.
          //
          strcpy_s(buffer_tail - error_message_length - 1,
                   error_message_length + 1,
                   error_message);
        }
        else
        {
          //
          // If error printing is disabled, just fail silently.
          //
          ;
        }

        return;
      }

      //
      // Compute carefully the maximum length of the message which can still
      // fit inside of the buffer and copy it.
      //
      message_length = std::min(message_length, unused_buffer_size);
      strncpy_s(buffer_tail, unused_buffer_size,
                message, message_length);

      //
      // Replace "\r\n" sequence at the end of the message with "\n\0" and
      // point the tail at the position AFTER the "\0", where we write another
      // "\0". In another words, this will effectively change the end of the
      // message from "\r\n\0" to "\n\0\0"
      //            +------------------^
      //            |
      //  updated buffer_tail
      //
      // The reason for this is that if there has been leftover from some old
      // message, it will be overwriten by "\0". If we wouldn't do that, the
      // loop inside of the flush() function might think there is another string
      // in the buffer and read past the tail.
      //
      auto location_of_cr = message_length - 2;
      buffer_tail[location_of_cr + 0] = '\n'; // '\r' -> '\n'
      buffer_tail[location_of_cr + 1] = '\0'; // '\n' -> '\0'
      buffer_tail[location_of_cr + 2] = '\0'; //  <?> -> '\0'

      buffer_tail += location_of_cr + 2;
    }

    void do_print(const char* message) noexcept
    {
      DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%s", message);
    }

    char* swap_buffers() noexcept
    {
      //
      // Lock the buffers so nobody can write any further messages to the buffer
      // while we're swapping it.
      //
      std::lock_guard _(lock);
   
      //
      // Save the current buffer.
      //
      auto old_buffer = buffer_head;

      //
      // If the buffer is empty, we have nothing to print.
      //
      if (old_buffer[0] == '\0')
      {
        return nullptr;
      }

      //
      // Finally, swap the buffers and set tail of the buffer to head (begin)
      // of the buffer.
      //
      buffer_head = (old_buffer == buffer1) ? buffer2 : buffer1;
      buffer_tail = buffer_head;

      //
      // ...and make the new buffer empty.
      //
      buffer_head[0] = '\0';

      //
      // ...and reset enqueue fail counter.
      //
      enqueue_fail_count = 0;

      return old_buffer;
    }

    void flush() noexcept
    {
      char* old_buffer = swap_buffers();

      //
      // Loop through each log message. Note that we don't need to be under a lock
      // anymore, since flush is called only from single thread.
      //
      while (old_buffer && *old_buffer)
      {
        //
        // Print each message.
        //
        do_print(old_buffer);

        //
        // Advance the buffer.
        //
        auto message_length = strlen(old_buffer);
        old_buffer += message_length + 1;
      }
    }

    void flush_thread_routine(void*) noexcept
    {
      thread_alive = true;

      //
      // Flush the buffer in a loop and wait between each flush little time.
      //
      while (thread_alive)
      {
        flush();

        mp::sleep(flush_interval_ms);
      }
    }
  }

  void initialize(size_t size) noexcept
  {
    buffer_size = size;

    //
    // Allocate buffers for double buffering and terminate them as a string.
    //
    buffer1 = new char[size];
    buffer2 = new char[size];
    buffer1[0] = '\0';
    buffer2[0] = '\0';

    //
    // Initialize current buffer as a buffer1.
    //
    buffer_head = buffer1;
    buffer_tail = buffer1;

    //
    // Initialize level and options.
    //
    current_level = level_t::default;
    current_options = options_t::default;

    //
    // Initialize buffer lock.
    //
    new (&lock) spinlock();

    //
    // Finally, create flushing thread.
    //
    PsCreateSystemThread(&thread_handle,
                         THREAD_ALL_ACCESS,
                         nullptr,
                         nullptr,
                         nullptr,
                         detail::flush_thread_routine,
                         nullptr);
  }

  void destroy() noexcept
  {
    //
    // Destroy the flush thread.
    //
    if (thread_handle != nullptr)
    {
      thread_alive = false;
      ZwWaitForSingleObject(thread_handle, FALSE, nullptr);
      ZwClose(thread_handle);

      thread_handle = nullptr;
    }

    //
    // Destroy both buffers.
    //
    if (buffer1 != nullptr)
    {
      delete[] buffer1;
      buffer1 = nullptr;
    }

    if (buffer2 != nullptr)
    {
      delete[] buffer2;
      buffer2 = nullptr;
    }

    //
    // Reset all values.
    //
    buffer_size = 0;
    buffer_head = nullptr;
    buffer_tail = nullptr;
    enqueue_fail_count = 0;
  }

  void set_options(options_t opt) noexcept
  {
    current_options = opt;
  }

  auto get_options() noexcept -> options_t
  {
    return current_options;
  }

  void set_level(level_t level) noexcept
  {
    current_level = level;
  }

  auto get_level() noexcept -> level_t
  {
    return current_level;
  }

  void print(level_t level, const char* function, const char* format, ...) noexcept
  {
    //
    // Quickly exit if we don't log at provided level.
    //
    if (!detail::test_level(level))
    {
      return;
    }

    //
    // Format the final message and enqueue it.
    //
    char message[512];
    int message_length;

    va_list args;
    va_start(args, format);
    message_length = detail::vprint(message, level, function, format, args);
    va_end(args);

    //
    // Check if we have to enqueue the message or if we can print it immediately.
    //
    if (detail::test_option(options_t::print_buffered))
    {
      detail::enqueue_message(message, message_length);
    }
    else
    {
      detail::do_print(message);
    }
  }
}
