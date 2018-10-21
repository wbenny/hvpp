#pragma once
#include <hvpp/lib/list.h>
#include <hvpp/lib/error.h>

namespace monitor
{
  auto initialize() noexcept -> error_code_t;
  void destroy() noexcept;

  auto start(uint64_t pid) noexcept -> error_code_t;
  void stop() noexcept;

  bool is_started() noexcept;

  bool is_monitored_pid(uint64_t pid) noexcept;

  //
  // Notification functions.
  //

  void on_process_create(uint64_t pid, uint64_t parent_pid) noexcept;
  void on_process_terminate(uint64_t pid) noexcept;
}
