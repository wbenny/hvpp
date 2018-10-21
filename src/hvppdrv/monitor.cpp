#include "monitor.h"

#include "win32/inject.h"
#include "win32/monitor.h"
#include "win32/trace.h"

#include <hvpp/lib/assert.h>
#include <hvpp/lib/object.h>
#include <hvpp/lib/log.h>

#include <ntddk.h>

namespace monitor
{
  //
  // Monitor implementation.
  //

  object_t<list<uint64_t>> monitored_pid_list;
  bool                     monitor_all_processes;
  bool                     started;

  //
  // WDK -> C++ glue functions.
  //

  VOID
  NTAPI
  GlueProcessNotify(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
    )
  {
    UNREFERENCED_PARAMETER(Process);

    CreateInfo
      ? on_process_create(uint64_t(ProcessId), uint64_t(CreateInfo->ParentProcessId))
      : on_process_terminate(uint64_t(ProcessId));
  };

  VOID
  NTAPI
  GlueLoadImageNotify(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
    )
  {
    InjLoadImageNotifyRoutine(FullImageName, ProcessId, ImageInfo);
  };

  //
  // Implementation.
  //

  auto initialize() noexcept -> error_code_t
  {
    monitored_pid_list.initialize();
    monitor_all_processes = false;
    started = false;

    //
    // Register this instance as active.
    //

    return error_code_t{};
  }

  void destroy() noexcept
  {
    if (started)
    {
      stop();
    }

    monitored_pid_list.destroy();
  }

  auto start(uint64_t pid) noexcept -> error_code_t
  {
    hvpp_assert(!started);

    //
    // Monitor all processes if PID == 0.
    //

    monitor_all_processes = (pid == 0);

    //
    // Add specified PID to the list of monitored
    // processes.
    //

    if (!monitor_all_processes)
    {
      monitored_pid_list->push_back(pid);
    }

    //
    // Register notification routines.
    //

#define INJ_CUSTOM_PATH L"C:\\Users\\John\\Desktop\\"
    UNICODE_STRING DllPathX86 = RTL_CONSTANT_STRING(INJ_CUSTOM_PATH L"hvpp\\Debug\\hvppdllx64.dll");
    UNICODE_STRING DllPathX64 = RTL_CONSTANT_STRING(INJ_CUSTOM_PATH L"hvpp\\Debug\\hvppdllx64.dll");
    BOOLEAN UseWow64Injection = FALSE;

    NTSTATUS status;

    status = TraceInitialize();
    status = MonInitialize(&GlueProcessNotify, nullptr, &GlueLoadImageNotify);
    status = InjInitialize(&DllPathX86,
                           &DllPathX64,
                           UseWow64Injection);


    //
    // Signalize that the monitor is started.
    //

    started = true;

    return error_code_t{};
  }

  void stop() noexcept
  {
    hvpp_assert(started);

    if (!started)
    {
      return;
    }

    //
    // Unregister notification routines.
    //

    InjDestroy();
    MonDestroy();
    TraceDestroy();

    //
    // Clear list of monitored processes.
    //

    monitored_pid_list->clear();

    //
    // Signalize that the monitor is stopped.
    //

    started = false;
  }

  bool is_started() noexcept
  {
    return started;
  }

  bool is_monitored_pid(uint64_t pid) noexcept
  {
    //
    // Return true if all processes are monitored
    // or if the PID is in the list of monitored
    // processes.
    //

    return monitor_all_processes
        || std::find(monitored_pid_list->begin(),
                     monitored_pid_list->end(),
                     pid) != monitored_pid_list->end();
  }

  //
  // Notification functions.
  //

  void on_process_create(uint64_t pid, uint64_t parent_pid) noexcept
  {
    if (is_monitored_pid(parent_pid))
    {
      //
      // Parent is monitored - add the child to the
      // list.
      //

      hvpp_info("Adding PID %u to the monitored list", (uint32_t)pid);

      monitored_pid_list->push_back(pid);
      InjCreateInjectionInfo((HANDLE)pid);
    }
  }

  void on_process_terminate(uint64_t pid) noexcept
  {
    monitored_pid_list->remove(pid);
    InjRemoveInjectionInfo((HANDLE)pid);

    if (monitored_pid_list->empty())
    {
      hvpp_info("Monitored PID list empty");
    }
  }
}
