#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1
#include <windows.h>

#include "DetoursNT.h"

// #define DETOURSNT_NO_EH

#ifdef __cplusplus
extern "C" {
#endif

#pragma region CRT

#pragma function(memset)
void* __cdecl
memset(
  void* dest,
  int ch,
  size_t count
  )
{
  if (count)
  {
    char* d = (char*)dest;

    do
    {
      *d++ = ch;
    } while (--count);
  }

  return dest;
}

#pragma function(memcpy)
void* __cdecl
memcpy(
  void* dest,
  const void* src,
  size_t count
  )
{
  char* d = (char*)dest;
  char* s = (char*)src;

  while (count--)
  {
    *d++ = *s++;
  }

  return dest;
}

//
// TODO: Use RtlCreateHeap/RtlAllocateHeap/RtlFreeHeap?
//

void* __cdecl
malloc(
  size_t size
  )
{
  return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void* __cdecl
calloc(
  size_t count,
  size_t size
  )
{
  //
  // UNIMPLEMENTED
  //
  return NULL;
}

void* __cdecl
realloc(
  void* ptr,
  size_t new_size
  )
{
  //
  // UNIMPLEMENTED
  //
  return NULL;
}

void __cdecl
free(
  void* ptr
  )
{
  VirtualFree(ptr, 0, MEM_RELEASE);
}

#ifdef __cplusplus

} // extern "C"

void* __cdecl
operator new(
  size_t size
  )
{
  return malloc(size);
}

void* __cdecl
operator new[](
  size_t size
  )
{
  return malloc(size);
}

void __cdecl
operator delete(
  void* pointer
  ) noexcept
{
  free(pointer);
}

void __cdecl
operator delete(
  void* pointer,
  size_t
  ) noexcept
{
  free(pointer);
}

void __cdecl
operator delete[](
  void* pointer
  ) noexcept
{
  free(pointer);
}

void __cdecl
operator delete[](
  void* pointer,
  size_t
  ) noexcept
{
  free(pointer);
}

extern "C" {

#endif

#ifndef DETOURSNT_NO_EH

typedef struct _ANSI_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PSTR   Buffer;
} ANSI_STRING;
typedef ANSI_STRING *PANSI_STRING;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }

NTSTATUS
NTAPI
LdrLoadDll(
  IN PWSTR SearchPath OPTIONAL,
  IN PULONG DllCharacteristics OPTIONAL,
  IN PUNICODE_STRING DllName,
  OUT PVOID *BaseAddress
  );

NTSTATUS
NTAPI
LdrGetProcedureAddress(
  IN PVOID BaseAddress,
  IN PANSI_STRING Name,
  IN ULONG Ordinal,
  OUT PVOID *ProcedureAddress
  );

#ifdef _M_AMD64

EXCEPTION_DISPOSITION
__cdecl
__C_specific_handler(
  _In_ struct _EXCEPTION_RECORD* ExceptionRecord,
  _In_ void* EstablisherFrame,
  _Inout_ struct _CONTEXT* ContextRecord,
  _Inout_ struct _DISPATCHER_CONTEXT* DispatcherContext
  )
{
#if 0
  typedef EXCEPTION_DISPOSITION (__cdecl * pfn__C_specific_handler)(
    _In_ struct _EXCEPTION_RECORD* ExceptionRecord,
    _In_ void* EstablisherFrame,
    _Inout_ struct _CONTEXT* ContextRecord,
    _Inout_ struct _DISPATCHER_CONTEXT* DispatcherContext
    );

  static pfn__C_specific_handler Procedure = NULL;

  if (!Procedure)
  {
    PVOID NtdllImageBase;
    RtlPcToFileHeader((PVOID)NtAllocateVirtualMemory, &NtdllImageBase);

    ANSI_STRING ProcedureName = RTL_CONSTANT_STRING("__C_specific_handler");
    LdrGetProcedureAddress(NtdllImageBase, &ProcedureName, 0, (PVOID*)&Procedure);

    //
    // Do not check for errors, if Procedure remains NULL, we're screwed anyway.
    //
  }

  return Procedure(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext);
#else
#ifdef _DEBUG
  __debugbreak();
#endif

  *reinterpret_cast<void**>(nullptr) = nullptr;

  return (EXCEPTION_DISPOSITION)0;
#endif
}

#endif // _M_IA64

#if _M_IX86

int
_callnewh(
  size_t size
  )
{
  return 0;
}

EXCEPTION_DISPOSITION
__cdecl
_except_handler3(
  _In_ struct _EXCEPTION_RECORD* ExceptionRecord,
  _In_ void* EstablisherFrame,
  _Inout_ struct _CONTEXT* ContextRecord,
  _Inout_ struct _DISPATCHER_CONTEXT* DispatcherContext
  )
{
#ifdef _DEBUG
  __debugbreak();
#endif

  *reinterpret_cast<void**>(nullptr) = nullptr;

  return (EXCEPTION_DISPOSITION)0;
}

#endif // _M_IX86

#endif // DETOURSNT_NO_EH

#pragma endregion

#pragma region Detours/module.cpp

ULONG WINAPI DetourGetModuleSize(_In_opt_ HMODULE hModule)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        // pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
        pDosHeader = &__ImageBase;
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return 0;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return 0;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return 0;
        }
        SetLastError(NO_ERROR);

        return (pNtHeader->OptionalHeader.SizeOfImage);
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return 0;
    }
}

#pragma endregion

#pragma region KERNEL32.DLL

#define NT_SUCCESS(Status)        (((NTSTATUS)(Status)) >= 0)

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
  _In_ NTSTATUS Status
  );

#define _CLIENT_ID  Mock__CLIENT_ID
#define CLIENT_ID   Mock_CLIENT_ID
#define PCLIENT_ID  Mock_PCLIENT_ID

// #define _TEB        Mock__TEB
// #define TEB         Mock_TEB
// #define PTEB        Mock_PTEB

typedef struct _CLIENT_ID
{
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _TEB
{
  NT_TIB    Tib;
  PVOID     EnvironmentPointer;
  CLIENT_ID ClientId;
} TEB, *PTEB;

static DWORD g_LastError = 0;

DWORD
WINAPI
GetLastError(
  VOID
  )
{
  return g_LastError;
}

VOID
WINAPI
SetLastError(
  IN DWORD dwErrCode
  )
{
  g_LastError = dwErrCode;
}

DWORD
WINAPI
BaseSetLastNTError(IN NTSTATUS Status)
{
  DWORD dwErrCode;

  /* Convert from NT to Win32, then set */
  dwErrCode = RtlNtStatusToDosError(Status);
  SetLastError(dwErrCode);
  return dwErrCode;
}

DWORD
WINAPI
GetCurrentThreadId(
  VOID
  )
{
  return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

LPVOID
NTAPI
VirtualAlloc(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flAllocationType,
  IN DWORD flProtect
  )
{
  /* Call the extended API */
  return VirtualAllocEx(GetCurrentProcess(),
                        lpAddress,
                        dwSize,
                        flAllocationType,
                        flProtect);
}

LPVOID
NTAPI
VirtualAllocEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flAllocationType,
  IN DWORD flProtect
  )
{
  NTSTATUS Status;

  /* Handle any possible exceptions */
  __try
  {
    /* Allocate the memory */
    Status = NtAllocateVirtualMemory(hProcess,
                                     &lpAddress,
                                     0,
                                     &dwSize,
                                     flAllocationType,
                                     flProtect);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Status = GetExceptionCode();
  }

  /* Check for status */
  if (!NT_SUCCESS(Status))
  {
    /* We failed */
    BaseSetLastNTError(Status);
    return NULL;
  }

  /* Return the allocated address */
  return lpAddress;
}

BOOL
NTAPI
VirtualFree(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD dwFreeType
  )
{
  /* Call the extended API */
  return VirtualFreeEx(GetCurrentProcess(),
                       lpAddress,
                       dwSize,
                       dwFreeType);
}

BOOL
NTAPI
VirtualFreeEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD dwFreeType
  )
{
  NTSTATUS Status;

  /* Validate size and flags */
  if (!(dwSize) || !(dwFreeType & MEM_RELEASE))
  {
    /* Free the memory */
    Status = NtFreeVirtualMemory(hProcess,
                                 &lpAddress,
                                 &dwSize,
                                 dwFreeType);
    if (!NT_SUCCESS(Status))
    {
      /* We failed */
      BaseSetLastNTError(Status);
      return FALSE;
    }

    /* Return success */
    return TRUE;
  }

  /* Invalid combo */
  BaseSetLastNTError(STATUS_INVALID_PARAMETER);
  return FALSE;
}

BOOL
NTAPI
VirtualProtect(
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flNewProtect,
  OUT PDWORD lpflOldProtect
  )
{
  /* Call the extended API */
  return VirtualProtectEx(GetCurrentProcess(),
                          lpAddress,
                          dwSize,
                          flNewProtect,
                          lpflOldProtect);
}

BOOL
NTAPI
VirtualProtectEx(
  IN HANDLE hProcess,
  IN LPVOID lpAddress,
  IN SIZE_T dwSize,
  IN DWORD flNewProtect,
  OUT PDWORD lpflOldProtect
  )
{
  NTSTATUS Status;

  /* Change the protection */
  Status = NtProtectVirtualMemory(hProcess,
                                  &lpAddress,
                                  &dwSize,
                                  flNewProtect,
                                  (PULONG)lpflOldProtect);
  if (!NT_SUCCESS(Status))
  {
    /* We failed */
    BaseSetLastNTError(Status);
    return FALSE;
  }

  /* Return success */
  return TRUE;
}

SIZE_T
NTAPI
VirtualQuery(
  IN LPCVOID lpAddress,
  OUT PMEMORY_BASIC_INFORMATION lpBuffer,
  IN SIZE_T dwLength
  )
{
  /* Call the extended API */
  return VirtualQueryEx(NtCurrentProcess(),
                        lpAddress,
                        lpBuffer,
                        dwLength);
}

SIZE_T
NTAPI
VirtualQueryEx(
  IN HANDLE hProcess,
  IN LPCVOID lpAddress,
  OUT PMEMORY_BASIC_INFORMATION lpBuffer,
  IN SIZE_T dwLength
  )
{
  NTSTATUS Status;
  SIZE_T ResultLength;

  /* Query basic information */
  Status = NtQueryVirtualMemory(hProcess,
                                (LPVOID)lpAddress,
                                MemoryBasicInformation,
                                lpBuffer,
                                dwLength,
                                &ResultLength);
  if (!NT_SUCCESS(Status))
  {
    /* We failed */
    BaseSetLastNTError(Status);
    return 0;
  }

  /* Return the length returned */
  return ResultLength;
}

BOOL
NTAPI
ReadProcessMemory(
  IN HANDLE hProcess,
  IN LPCVOID lpBaseAddress,
  IN LPVOID lpBuffer,
  IN SIZE_T nSize,
  OUT SIZE_T* lpNumberOfBytesRead
  )
{
  NTSTATUS Status;

  /* Do the read */
  Status = NtReadVirtualMemory(hProcess,
                               (PVOID)lpBaseAddress,
                               lpBuffer,
                               nSize,
                               &nSize);

  /* In user-mode, this parameter is optional */
  if (lpNumberOfBytesRead) *lpNumberOfBytesRead = nSize;
  if (!NT_SUCCESS(Status))
  {
    /* We failed */
    BaseSetLastNTError(Status);
    return FALSE;
  }

  /* Return success */
  return TRUE;
}

BOOL
WINAPI
FlushInstructionCache(
  IN HANDLE hProcess,
  IN LPCVOID lpBaseAddress,
  IN SIZE_T nSize
  )
{
  NTSTATUS Status;

  /* Call the native function */
  Status = NtFlushInstructionCache(hProcess, (PVOID)lpBaseAddress, nSize);
  if (!NT_SUCCESS(Status))
  {
    /* Handle failure case */
    BaseSetLastNTError(Status);
    return FALSE;
  }

  /* All good */
  return TRUE;
}

DWORD
WINAPI
SuspendThread(
  IN HANDLE hThread
  )
{
  ULONG PreviousSuspendCount;
  NTSTATUS Status;

  Status = NtSuspendThread(hThread, &PreviousSuspendCount);
  if (!NT_SUCCESS(Status))
  {
    BaseSetLastNTError(Status);
    return -1;
  }

  return PreviousSuspendCount;
}

DWORD
WINAPI
ResumeThread(
  IN HANDLE hThread
  )
{
  ULONG PreviousResumeCount;
  NTSTATUS Status;

  Status = NtResumeThread(hThread, &PreviousResumeCount);
  if (!NT_SUCCESS(Status))
  {
    BaseSetLastNTError(Status);
    return -1;
  }

  return PreviousResumeCount;
}

BOOL
WINAPI
GetThreadContext(
  IN HANDLE hThread,
  OUT LPCONTEXT lpContext
  )
{
  NTSTATUS Status;

  Status = NtGetContextThread(hThread, lpContext);
  if (!NT_SUCCESS(Status))
  {
    BaseSetLastNTError(Status);
    return FALSE;
  }

  return TRUE;
}

BOOL
WINAPI
SetThreadContext(
  IN HANDLE hThread,
  IN CONST CONTEXT *lpContext
  )
{
  NTSTATUS Status;

  Status = NtSetContextThread(hThread, (PCONTEXT)lpContext);
  if (!NT_SUCCESS(Status))
  {
    BaseSetLastNTError(Status);
    return FALSE;
  }

  return TRUE;
}

#ifdef __cplusplus
}
#endif
