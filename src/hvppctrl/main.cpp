#include <cstdio>
#include <cstdint>

#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>

#include "../hvpp/hvpp/lib/ioctl.h"

using ioctl_monitor_start_t     = ioctl_read_t<1, sizeof(uint64_t)>;
using ioctl_monitor_stop_t      = ioctl_none_t<1>;

using ioctl_hypervisor_start_t  = ioctl_none_t<2>;
using ioctl_hypervisor_stop_t   = ioctl_none_t<3>;

void EnableMonitor()
{
  HANDLE DeviceHandle;

  DeviceHandle = CreateFile(TEXT("\\\\.\\hvpp"),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

  if (DeviceHandle == INVALID_HANDLE_VALUE)
  {
    printf("Error while opening 'hvpp' device!\n");
    return;
  }

  HANDLE ProcessId = (HANDLE)(ULONG_PTR)GetCurrentProcessId();
  DWORD BytesReturned;
  DeviceIoControl(DeviceHandle,
                  ioctl_monitor_start_t::code(),
                  &ProcessId,
                  sizeof(ProcessId),
                  NULL,
                  0,
                  &BytesReturned,
                  NULL);

  CloseHandle(DeviceHandle);
}

//////////////////////////////////////////////////////////////////////////

//
// GUID:
//   {a4b4ba50-a667-43f5-919b-1e52a6d69bd5}
//

GUID ProviderGuid = {
  0xa4b4ba50, 0xa667, 0x43f5, { 0x91, 0x9b, 0x1e, 0x52, 0xa6, 0xd6, 0x9b, 0xd5 }
};

//
// GUID:
//   {53d82d11-cede-4dff-8eb4-f06631800128}
//

GUID SessionGuid = {
  0x53d82d11, 0xcede, 0x4dff, { 0x8e, 0xb4, 0xf0, 0x66, 0x31, 0x80, 0x1, 0x28 }
};

TCHAR SessionName[] = TEXT("InjSession");

VOID
WINAPI
TraceEventCallback(
  _In_ PEVENT_RECORD EventRecord
  )
{
  if (!EventRecord->UserData)
  {
    return;
  }

  //
  // TODO: Check that EventRecord contains only WCHAR string.
  //

  wprintf(L"[PID:%04X][TID:%04X] %s\n",
          EventRecord->EventHeader.ProcessId,
          EventRecord->EventHeader.ThreadId,
          (PWCHAR)EventRecord->UserData);
}

ULONG
NTAPI
TraceStart(
  VOID
  )
{
  //
  // Start new trace session.
  // For an awesome blogpost on ETW API, see:
  // https://caseymuratori.com/blog_0025
  //

  ULONG ErrorCode;

  TRACEHANDLE TraceSessionHandle = INVALID_PROCESSTRACE_HANDLE;
  TRACEHANDLE TraceHandle = INVALID_PROCESSTRACE_HANDLE;
  EVENT_TRACE_LOGFILE TraceLogfile = { 0 };

  BYTE Buffer[sizeof(EVENT_TRACE_PROPERTIES) + 4096];
  RtlZeroMemory(Buffer, sizeof(Buffer));

  PEVENT_TRACE_PROPERTIES EventTraceProperties = (PEVENT_TRACE_PROPERTIES)Buffer;
  EventTraceProperties->Wnode.BufferSize = sizeof(Buffer);

  RtlZeroMemory(Buffer, sizeof(Buffer));
  EventTraceProperties->Wnode.BufferSize = sizeof(Buffer);
  EventTraceProperties->Wnode.ClientContext = 1; // Use QueryPerformanceCounter, see MSDN
  EventTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
  EventTraceProperties->LogFileMode = PROCESS_TRACE_MODE_REAL_TIME;
  EventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

  ErrorCode = StartTrace(&TraceSessionHandle, SessionName, EventTraceProperties);
  if (ErrorCode != ERROR_SUCCESS)
  {
    goto Exit;
  }

  //
  // Enable tracing of our provider.
  //

  ErrorCode = EnableTrace(TRUE, 0, 0, &ProviderGuid, TraceSessionHandle);
  if (ErrorCode != ERROR_SUCCESS)
  {
    goto Exit;
  }

  TraceLogfile.LoggerName = SessionName;
  TraceLogfile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;
  TraceLogfile.EventRecordCallback = &TraceEventCallback;

  //
  // Open real-time tracing session.
  //

  TraceHandle = OpenTrace(&TraceLogfile);
  if (TraceHandle == INVALID_PROCESSTRACE_HANDLE)
  {
    //
    // Synthetic error code.
    //
    ErrorCode = ERROR_FUNCTION_FAILED;
    goto Exit;
  }

  //
  // Process trace events.  This call is blocking.
  //

  ErrorCode = ProcessTrace(&TraceHandle, 1, NULL, NULL);

Exit:
  if (TraceHandle != INVALID_PROCESSTRACE_HANDLE)
  {
    CloseTrace(TraceHandle);
  }

  if (TraceSessionHandle != INVALID_PROCESSTRACE_HANDLE)
  {
    CloseTrace(TraceSessionHandle);
  }

  RtlZeroMemory(Buffer, sizeof(Buffer));
  EventTraceProperties->Wnode.BufferSize = sizeof(Buffer);
  StopTrace(0, SessionName, EventTraceProperties);

  if (ErrorCode != ERROR_SUCCESS)
  {
    printf("Error: %08x\n", ErrorCode);
  }

  return ErrorCode;
}

VOID
NTAPI
TraceStop(
  VOID
  )
{
  BYTE Buffer[sizeof(EVENT_TRACE_PROPERTIES) + 4096];
  RtlZeroMemory(Buffer, sizeof(Buffer));

  PEVENT_TRACE_PROPERTIES EventTraceProperties = (PEVENT_TRACE_PROPERTIES)Buffer;
  EventTraceProperties->Wnode.BufferSize = sizeof(Buffer);

  StopTrace(0, SessionName, EventTraceProperties);
}

//////////////////////////////////////////////////////////////////////////

BOOL
WINAPI
CtrlCHandlerRoutine(
  _In_ DWORD dwCtrlType
  )
{
  if (dwCtrlType == CTRL_C_EVENT)
  {
    //
    // Ctrl+C was pressed, stop the trace session.
    //
    printf("Ctrl+C pressed, stopping trace session...\n");

    TraceStop();
  }

  return FALSE;
}

DWORD
WINAPI
TraceThreadRoutine(
  LPVOID lpParam
  )
{
  //
  // Stop any previous trace session (if exists).
  //

  TraceStop();

  printf("Starting tracing session...\n");

  ULONG ErrorCode = TraceStart();

  return ErrorCode;
}

int main()
{
  //LoadLibrary(TEXT("hvppdllx64.dll"));

  SetConsoleCtrlHandler(&CtrlCHandlerRoutine, TRUE);

  HANDLE TraceHandle = CreateThread(NULL, 0, &TraceThreadRoutine, NULL, 0, NULL);
  Sleep(100);

  EnableMonitor();
  //system("start notepad");

  STARTUPINFO StartupInfo = { 0 };
  PROCESS_INFORMATION ProcessInformation;

  StartupInfo.cb = sizeof(StartupInfo);

  CreateProcess(TEXT("C:\\windows\\notepad.exe"),   // Name of program to execute
                NULL,                  // Command line
                NULL,                  // Process handle not inheritable
                NULL,                  // Thread handle not inheritable
                FALSE,                 // Set handle inheritance to FALSE
                0,                     // No creation flags
                NULL,                  // Use parent's environment block
                NULL,                  // Use parent's starting directory
                &StartupInfo,          // Pointer to STARTUPINFO structure
                &ProcessInformation);  // Pointer to PROCESS_INFORMATION structure

  CloseHandle(ProcessInformation.hProcess);
  CloseHandle(ProcessInformation.hThread);

  printf("Waiting to stop...\n");
  WaitForSingleObject(TraceHandle, INFINITE);

  //LoadLibrary(TEXT("hvppdllx64.dll"));

  return 0;
}

