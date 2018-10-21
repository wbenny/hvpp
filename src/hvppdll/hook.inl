#include "log.h"

HOOK_DEFINE(NtQuerySystemInformation);

NTSTATUS
NTAPI
HookNtQuerySystemInformation(
  _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
  _In_ ULONG SystemInformationLength,
  _Out_opt_ PULONG ReturnLength
  )
{
  Log(L"NtQuerySystemInformation(%i, %p, %i)",
      SystemInformationClass,
      SystemInformation,
      SystemInformationLength);

  return OrigNtQuerySystemInformation(SystemInformationClass,
                                      SystemInformation,
                                      SystemInformationLength,
                                      ReturnLength);
}

// HOOK_DEFINE(NtCreateThreadEx);
// 
// NTSTATUS
// NTAPI
// HookNtCreateThreadEx(
//   _Out_ PHANDLE ThreadHandle,
//   _In_ ACCESS_MASK DesiredAccess,
//   _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
//   _In_ HANDLE ProcessHandle,
//   _In_ PVOID StartRoutine, // PUSER_THREAD_START_ROUTINE
//   _In_opt_ PVOID Argument,
//   _In_ ULONG CreateFlags, // THREAD_CREATE_FLAGS_*
//   _In_ SIZE_T ZeroBits,
//   _In_ SIZE_T StackSize,
//   _In_ SIZE_T MaximumStackSize,
//   _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
//   )
// {
//   Log(L"NtCreateThreadEx(%p, %p)",
//       ProcessHandle,
//       StartRoutine);
// 
//   return OrigNtCreateThreadEx(ThreadHandle,
//                               DesiredAccess,
//                               ObjectAttributes,
//                               ProcessHandle,
//                               StartRoutine,
//                               Argument,
//                               CreateFlags,
//                               ZeroBits,
//                               StackSize,
//                               MaximumStackSize,
//                               AttributeList);
// }
