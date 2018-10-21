#include "inject.h"

#include <ntddk.h>
#include <ntimage.h>

//////////////////////////////////////////////////////////////////////////
// ke.h
//////////////////////////////////////////////////////////////////////////

typedef enum _KAPC_ENVIRONMENT
{
  OriginalApcEnvironment,
  AttachedApcEnvironment,
  CurrentApcEnvironment,
  InsertApcEnvironment
} KAPC_ENVIRONMENT;

typedef
VOID
(NTAPI *PKNORMAL_ROUTINE) (
  _In_ PVOID NormalContext,
  _In_ PVOID SystemArgument1,
  _In_ PVOID SystemArgument2
  );

typedef
VOID
(NTAPI *PKKERNEL_ROUTINE) (
  _In_ PKAPC Apc,
  _Inout_ PKNORMAL_ROUTINE* NormalRoutine,
  _Inout_ PVOID* NormalContext,
  _Inout_ PVOID* SystemArgument1,
  _Inout_ PVOID* SystemArgument2
  );

typedef
VOID
(NTAPI *PKRUNDOWN_ROUTINE) (
  _In_ PKAPC Apc
  );

NTKERNELAPI
VOID
NTAPI
KeInitializeApc(
  _Out_ PRKAPC Apc,
  _In_ PETHREAD Thread,
  _In_ KAPC_ENVIRONMENT Environment,
  _In_ PKKERNEL_ROUTINE KernelRoutine,
  _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
  _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
  _In_opt_ KPROCESSOR_MODE ApcMode,
  _In_opt_ PVOID NormalContext
  );

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueApc(
  _Inout_ PRKAPC Apc,
  _In_opt_ PVOID SystemArgument1,
  _In_opt_ PVOID SystemArgument2,
  _In_ KPRIORITY Increment
  );

NTKERNELAPI
BOOLEAN
NTAPI
KeAlertThread(
  _Inout_ PKTHREAD Thread,
  _In_ KPROCESSOR_MODE AlertMode
  );

NTKERNELAPI
BOOLEAN
NTAPI
KeTestAlertThread(
  _In_ KPROCESSOR_MODE AlertMode
  );

//////////////////////////////////////////////////////////////////////////
// ps.h
//////////////////////////////////////////////////////////////////////////

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWow64Process(
  _In_ PEPROCESS Process
  );

NTKERNELAPI
PCHAR
NTAPI
PsGetProcessImageFileName (
  _In_ PEPROCESS Process
  );

NTKERNELAPI
BOOLEAN
NTAPI
PsIsProtectedProcess(
  _In_ PEPROCESS Process
  );

//////////////////////////////////////////////////////////////////////////
// ntrtl.h
//////////////////////////////////////////////////////////////////////////

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
  _In_ PVOID BaseOfImage,
  _In_ BOOLEAN MappedAsImage,
  _In_ USHORT DirectoryEntry,
  _Out_ PULONG Size
  );

//////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////

#define INJ_MEMORY_TAG ' jnI'

//////////////////////////////////////////////////////////////////////////
// Enumerations.
//////////////////////////////////////////////////////////////////////////

typedef enum _INJ_SYSTEM_DLL
{
  INJ_NOTHING_LOADED            = 0x00,
  INJ_SYSTEM32_NTDLL_LOADED     = 0x01,
  INJ_SYSTEM32_WOW64_LOADED     = 0x02,
  INJ_SYSTEM32_WOW64WIN_LOADED  = 0x04,
  INJ_SYSTEM32_WOW64CPU_LOADED  = 0x08,
  INJ_SYSWOW64_NTDLL_LOADED     = 0x10,
} INJ_SYSTEM_DLL;

//////////////////////////////////////////////////////////////////////////
// Structures.
//////////////////////////////////////////////////////////////////////////

typedef struct _INJ_SYSTEM_DLL_DESCRIPTOR
{
  UNICODE_STRING DllPath;
  INJ_SYSTEM_DLL Flag;
} INJ_SYSTEM_DLL_DESCRIPTOR, *PINJ_SYSTEM_DLL_DESCRIPTOR;

typedef struct _INJ_GLOBAL_DATA
{
  LIST_ENTRY      InjectionInfoListHead;

  UNICODE_STRING  DllPathX86;
  PWCHAR          DllPathBufferX86;

#if defined(_M_AMD64)
  UNICODE_STRING  DllPathX64;
  PWCHAR          DllPathBufferX64;

  BOOLEAN         UseWow64Injection;
#endif
} INJ_GLOBAL_DATA;

//////////////////////////////////////////////////////////////////////////
// Function prototypes.
//////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
InjpQueueApc(
  _In_ KPROCESSOR_MODE ApcMode,
  _In_ PKNORMAL_ROUTINE NormalRoutine,
  _In_ PVOID NormalContext,
  _In_ PVOID SystemArgument1,
  _In_ PVOID SystemArgument2
  );

VOID
NTAPI
InjpInjectApcNormalRoutine(
  _In_ PVOID NormalContext,
  _In_ PVOID SystemArgument1,
  _In_ PVOID SystemArgument2
  );

VOID
NTAPI
InjpInjectApcKernelRoutine(
  _In_ PKAPC Apc,
  _Inout_ PKNORMAL_ROUTINE* NormalRoutine,
  _Inout_ PVOID* NormalContext,
  _Inout_ PVOID* SystemArgument1,
  _Inout_ PVOID* SystemArgument2
  );

//////////////////////////////////////////////////////////////////////////
// Variables.
//////////////////////////////////////////////////////////////////////////

INJ_GLOBAL_DATA GlobalData;

INJ_SYSTEM_DLL_DESCRIPTOR SystemDlls[] = {
  { RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\ntdll.dll"), INJ_SYSTEM32_NTDLL_LOADED    },
#if defined(_M_AMD64)
  { RTL_CONSTANT_STRING(L"\\Windows\\System32\\wow64.dll"),    INJ_SYSTEM32_WOW64_LOADED    },
  { RTL_CONSTANT_STRING(L"\\Windows\\System32\\wow64win.dll"), INJ_SYSTEM32_WOW64WIN_LOADED },
  { RTL_CONSTANT_STRING(L"\\Windows\\System32\\wow64cpu.dll"), INJ_SYSTEM32_WOW64CPU_LOADED },
  { RTL_CONSTANT_STRING(L"\\SystemRoot\\SysWOW64\\ntdll.dll"), INJ_SYSWOW64_NTDLL_LOADED    },
#endif
};

ANSI_STRING LdrLoadDllRoutineName = RTL_CONSTANT_STRING("LdrLoadDll");

//////////////////////////////////////////////////////////////////////////
// Helper functions.
//////////////////////////////////////////////////////////////////////////

PVOID
NTAPI
RtlxFindExportedRoutineByName(
  _In_ PVOID DllBase,
  _In_ PANSI_STRING ExportName
  )
{
  //
  // Borrowed from ReactOS.
  // Note that this function is not exported by ntoskrnl until Win10.
  //

  PULONG NameTable;
  PUSHORT OrdinalTable;
  PIMAGE_EXPORT_DIRECTORY ExportDirectory;
  LONG Low = 0, Mid = 0, High, Ret;
  USHORT Ordinal;
  PVOID Function;
  ULONG ExportSize;
  PULONG ExportTable;

  //
  // Get the export directory.
  //

  ExportDirectory = RtlImageDirectoryEntryToData(DllBase,
                                                 TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                 &ExportSize);

  if (!ExportDirectory)
  {
    return NULL;
  }

  //
  // Setup name tables.
  //

  NameTable    = (PULONG) ((ULONG_PTR)DllBase + ExportDirectory->AddressOfNames);
  OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase + ExportDirectory->AddressOfNameOrdinals);

  //
  // Do a binary search.
  //

  High = ExportDirectory->NumberOfNames - 1;
  while (High >= Low)
  {
    //
    // Get new middle value.
    //

    Mid = (Low + High) >> 1;

    //
    // Compare name.
    //

    Ret = strcmp(ExportName->Buffer, (PCHAR)DllBase + NameTable[Mid]);
    if (Ret < 0)
    {
      //
      // Update high.
      //
      High = Mid - 1;
    }
    else if (Ret > 0)
    {
      //
      // Update low.
      //
      Low = Mid + 1;
    }
    else
    {
      //
      // We got it.
      //
      break;
    }
  }

  //
  // Check if we couldn't find it.
  //

  if (High < Low)
  {
    return NULL;
  }

  //
  // Otherwise, this is the ordinal.
  //

  Ordinal = OrdinalTable[Mid];

  //
  // Validate the ordinal.
  //

  if (Ordinal >= ExportDirectory->NumberOfFunctions)
  {
    return NULL;
  }

  //
  // Resolve the address and write it.
  //

  ExportTable = (PULONG)((ULONG_PTR)DllBase + ExportDirectory->AddressOfFunctions);
  Function = (PVOID)((ULONG_PTR)DllBase + ExportTable[Ordinal]);

  //
  // We found it!
  //

  NT_ASSERT(
    (Function < (PVOID)ExportDirectory) ||
    (Function > (PVOID)((ULONG_PTR)ExportDirectory + ExportSize))
  );

  return Function;
}

//////////////////////////////////////////////////////////////////////////
// Private functions.
//////////////////////////////////////////////////////////////////////////

//
// Taken from ReactOS, used by InjpInitializeDllPaths.
//

typedef union
{
  WCHAR Name[sizeof(ULARGE_INTEGER) / sizeof(WCHAR)];
  ULARGE_INTEGER Alignment;
} ALIGNEDNAME;

//
// DOS Device Prefix \??\
//

ALIGNEDNAME ObpDosDevicesShortNamePrefix = { { L'\\', L'?', L'?', L'\\' } };
UNICODE_STRING ObpDosDevicesShortName = {
  sizeof(ObpDosDevicesShortNamePrefix), // Length
  sizeof(ObpDosDevicesShortNamePrefix), // MaximumLength
  (PWSTR)&ObpDosDevicesShortNamePrefix  // Buffer
};

NTSTATUS
NTAPI
InjpInitializeDllPaths(
  _In_ PUNICODE_STRING RegistryPath,
  _Inout_ PUNICODE_STRING DllPathX86,
  _Inout_ PUNICODE_STRING DllPathX64
  )
{
#if _M_IX86
  UNREFERENCED_PARAMETER(DllPathX64);
#endif

  //
  // In the "ImagePath" key of the RegistryPath, there
  // is a full path of this driver file.  Fetch it.
  //

  NTSTATUS Status;

  UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"ImagePath");

  OBJECT_ATTRIBUTES ObjectAttributes;
  InitializeObjectAttributes(&ObjectAttributes,
                             RegistryPath,
                             OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                             NULL,
                             NULL);

  HANDLE KeyHandle;
  Status = ZwOpenKey(&KeyHandle,
                     KEY_READ,
                     &ObjectAttributes);

  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

  //
  // Save all information on stack - simply fail if path
  // is too long.
  //

  UCHAR KeyValueInformationBuffer[sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(WCHAR) * 128];
  PKEY_VALUE_FULL_INFORMATION KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)KeyValueInformationBuffer;

  ULONG ResultLength;
  Status = ZwQueryValueKey(KeyHandle,
                           &ValueName,
                           KeyValueFullInformation,
                           KeyValueInformation,
                           sizeof(KeyValueInformationBuffer),
                           &ResultLength);

  ZwClose(KeyHandle);

  //
  // Check for succes.  Also check if the value is of expected type
  // and whether the path has a meaninful length.
  //

  if (!NT_SUCCESS(Status) ||
      KeyValueInformation->Type != REG_EXPAND_SZ ||
      KeyValueInformation->DataLength < sizeof(ObpDosDevicesShortNamePrefix))
  {
    return Status;
  }

  //
  // Save pointer to the fetched ImagePath value and test
  // if the path starts with "\??\" prefix - if so, skip it.
  //

  PWCHAR ImagePathValue = (PWCHAR)((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
  ULONG  ImagePathValueLength = KeyValueInformation->DataLength;

  if (*(PULONGLONG)(ImagePathValue) == ObpDosDevicesShortNamePrefix.Alignment.QuadPart)
  {
    ImagePathValue += ObpDosDevicesShortName.Length / sizeof(WCHAR);
    ImagePathValueLength -= ObpDosDevicesShortName.Length;
  }

  //
  // Find last occurence of the "\" character and if found,
  // save its position.
  //

  PWCHAR LastBackslash = wcsrchr(ImagePathValue, L'\\');

  if (!LastBackslash)
  {
    return STATUS_DATA_ERROR;
  }

  ULONG LastBackslashCharacterPosition = (ULONG)(LastBackslash - ImagePathValue);
  ULONG LastBackslashBytePosition = LastBackslashCharacterPosition * sizeof(WCHAR);

  //
  // Finally, fill the buffer...
  // Buffer is filled with the ImagePath up to the last
  // "\" cahracter (including), then the DLL name is appended.
  //

#define INJ_DLL_X86_NAME  L"injdllx86.dll"

  RtlCopyMemory(DllPathX86->Buffer,
                ImagePathValue,
                LastBackslashBytePosition + sizeof(WCHAR));

  if (DllPathX86->MaximumLength - LastBackslashCharacterPosition
      < sizeof(INJ_DLL_X86_NAME))
  {
    return STATUS_DATA_ERROR;
  }

  wcscpy(DllPathX86->Buffer + LastBackslashCharacterPosition + 1,
         INJ_DLL_X86_NAME);

  DllPathX86->Length = (USHORT)(LastBackslashBytePosition + sizeof(INJ_DLL_X86_NAME));

  DbgPrintEx(DPFLTR_IHVDRIVER_ID,
             DPFLTR_ERROR_LEVEL,
             "DLL x86 path: '%wZ'\n",
             DllPathX86);

#if _M_AMD64
  if (!DllPathX64)
  {
    return STATUS_PARTIAL_COPY;
  }

#define INJ_DLL_X64_NAME  L"injdllx64.dll"

  RtlCopyMemory(DllPathX64->Buffer,
                ImagePathValue,
                LastBackslashBytePosition + sizeof(WCHAR));

  if (DllPathX64->MaximumLength - LastBackslashCharacterPosition
      < sizeof(INJ_DLL_X64_NAME))
  {
    return STATUS_DATA_ERROR;
  }

  wcscpy(DllPathX64->Buffer + LastBackslashCharacterPosition + 1,
         INJ_DLL_X64_NAME);

  DllPathX64->Length = (USHORT)(LastBackslashBytePosition + sizeof(INJ_DLL_X64_NAME));

  DbgPrintEx(DPFLTR_IHVDRIVER_ID,
             DPFLTR_ERROR_LEVEL,
             "DLL x64 path: '%wZ'\n",
             DllPathX64);
#endif

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
InjpQueueApc(
  _In_ KPROCESSOR_MODE ApcMode,
  _In_ PKNORMAL_ROUTINE NormalRoutine,
  _In_ PVOID NormalContext,
  _In_ PVOID SystemArgument1,
  _In_ PVOID SystemArgument2
  )
{
  //
  // Allocate memory for the KAPC structure.
  //

  PKAPC Apc = ExAllocatePoolWithTag(NonPagedPoolNx,
                                    sizeof(KAPC),
                                    INJ_MEMORY_TAG);

  if (!Apc)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  //
  // Initialize and queue the APC.
  //

  KeInitializeApc(Apc,                                  // Apc
                  PsGetCurrentThread(),                 // Thread
                  OriginalApcEnvironment,               // Environment
                  &InjpInjectApcKernelRoutine,          // KernelRoutine
                  NULL,                                 // RundownRoutine
                  NormalRoutine,                        // NormalRoutine
                  ApcMode,                              // ApcMode
                  NormalContext);                       // NormalContext

  BOOLEAN Inserted = KeInsertQueueApc(Apc,              // Apc
                                      SystemArgument1,  // SystemArgument1
                                      SystemArgument2,  // SystemArgument2
                                      0);               // Increment

  if (!Inserted)
  {
    ExFreePoolWithTag(Apc, INJ_MEMORY_TAG);
    return STATUS_THREAD_IS_TERMINATING;
  }

  return STATUS_SUCCESS;
}

VOID
NTAPI
InjpInjectApcNormalRoutine(
  _In_ PVOID NormalContext,
  _In_ PVOID SystemArgument1,
  _In_ PVOID SystemArgument2
  )
{
  UNREFERENCED_PARAMETER(SystemArgument1);
  UNREFERENCED_PARAMETER(SystemArgument2);

  PINJ_INJECTION_INFO InjectionInfo = NormalContext;
  InjInject(InjectionInfo);
}

VOID
NTAPI
InjpInjectApcKernelRoutine(
  _In_ PKAPC Apc,
  _Inout_ PKNORMAL_ROUTINE* NormalRoutine,
  _Inout_ PVOID* NormalContext,
  _Inout_ PVOID* SystemArgument1,
  _Inout_ PVOID* SystemArgument2
  )
{
  UNREFERENCED_PARAMETER(NormalRoutine);
  UNREFERENCED_PARAMETER(NormalContext);
  UNREFERENCED_PARAMETER(SystemArgument1);
  UNREFERENCED_PARAMETER(SystemArgument2);

  //
  // Common kernel routine for both user-mode and
  // kernel-mode APCs queued by the InjpQueueApc
  // function.  Just release the memory of the APC
  // structure and return back.
  //

  ExFreePoolWithTag(Apc, INJ_MEMORY_TAG);
}

NTSTATUS
NTAPI
InjpInjectX86(
  _In_ PINJ_INJECTION_INFO InjectionInfo,
  _In_ HANDLE SectionHandle,
  _In_ SIZE_T SectionSize
  )
{
  NTSTATUS Status;

  NT_ASSERT(InjectionInfo->LdrLoadDllX86);

  UCHAR Shellcode[] = {
    //
    // ;++
    // ;
    // ; VOID
    // ; NTAPI
    // ; ApcNormalRoutine(
    // ;   _In_ PVOID NormalContext,
    // ;   _In_ PVOID SystemArgument1,
    // ;   _In_ PVOID SystemArgument2
    // ;   )
    // ;
    // ; Routine Description:
    // ;
    // ;    This routine loads DLL specified in the NormalContext.
    // ;
    // ;    On 32-bit Windows OS, this function is called from the
    // ;    ntdll.dll!KiUserApcDispatcher routine.
    // ;
    // ;    On 64-bit Windows OS, the following code-flow is responsible
    // ;    for reaching this function:
    // ;
    // ;    - wow64.dll!Wow64ApcRoutine (set by PsWrapApcWow64Thread)
    // ;      - Puts NormalRoutine, NormalContext, SystemArgument1 and SystemArgument2
    // ;        on the top of the stack, sets EIP to KiUserApcDispatcher of
    // ;        Wow64 ntdll.dll.
    // ;    - ntdll.dll!KiUserApcDispatcher (note this is Wow64 ntdll.dll)
    // ;      - Pops NormalRoutine - our ApcNormalRoutine - from the stack
    // ;        and calls it.
    // ;
    // ; Arguments:
    // ;
    // ;    NormalContext (ebp + 8) - Supplies following information:
    // ;
    // ;                                  struct /* INJ_APC_CONTEXT */ {
    // ;                                    /* +0x00 */ PVOID LdrLoadDll;
    // ;                                    /* +0x04 */ ULONG DllNameLength;
    // ;                                    /* +0x08 */ WCHAR DllNameBuffer[DllNameLength];
    // ;                                  };
    // ;
    // ;    SystemArgument1 - Ignored.
    // ;    SystemArgument2 - Ignored.
    // ;
    // ; Return Value:
    // ;
    // ;    None.
    // ;
    // ;--
    //
                            //
                            // _ApcNormalRoutine@12 PROC PUBLIC
                            //
    0x55,                   //     push   ebp
    0x89, 0xE5,             //     mov    ebp, esp
                            //
    0x83, 0xEC, 0x0c,       //     sub    esp, 12         ; reserve 12 bytes for local variables:
                            //                            ; UNICODE_STRING DllName; // (ebp - 12, size: 8)
                            //                            ; PVOID BaseAddress;      // (ebp - 4,  size: 4)
                            //
    0x8B, 0x4D, 0x08,       //     mov    ecx, [ebp + 8]  ; ecx =  NormalContext
    0x8B, 0x51, 0x04,       //     mov    edx, [ecx + 4]  ; edx =  NormalContext->DllNameLength
    0x8D, 0x41, 0x08,       //     lea    eax, [ecx + 8]  ; eax = &NormalContext->DllName
                            //
    0x66, 0x89, 0x55, 0xF4, //     mov    [ebp - 12], dx  ; DllName.Length        = dx  (NormalContext->DllNameLength)
    0x66, 0x89, 0x55, 0xF6, //     mov    [ebp - 10], dx  ; DllName.MaximumLength = dx  (NormalContext->DllNameLength)
    0x89, 0x45, 0xF8,       //     mov    [ebp -  8], eax ; DllName.Buffer        = eax (NormalContext->DllName)
                            //
    0x8D, 0x45, 0xFC,       //     lea    eax, [ebp -  4] ; eax = &BaseAddress (stack)
    0x8D, 0x55, 0xF4,       //     lea    edx, [ebp - 12] ; edx = &DllName
                            //
    0x50,                   //     push   eax
    0x52,                   //     push   edx
    0x6A, 0x00,             //     push   0
    0x6A, 0x00,             //     push   0
    0xFF, 0x11,             //     call   [ecx]           ; NormalContext->LdrLoadDll(NULL, 0, &DllName, &BaseAddress);
                            //
    0x89, 0xEC,             //     mov    esp, ebp
    0x5D,                   //     pop    ebp
    0xC2, 0x0C, 0x00        //     ret    12
                            //
  };                        // _ApcNormalRoutine@12 ENDP
                            //

  //
  // Structure holding the NormalContext.
  // Note that content of this structure will be processed by
  // the 32-bit code, meanwhile this source code can be compiled
  // by the 64-bit compiler.  This structure is packed so that
  // compiler won't make any undesired alignment decisions.
  //

  #pragma pack(push,1)
  typedef struct _INJ_APC_CONTEXT
  {
    /* +0x00 */ ULONG LdrLoadDll;
    /* +0x04 */ ULONG DllNameLength;
    /* +0x08 */ WCHAR DllNameBuffer[1];
  } INJ_APC_CONTEXT, *PINJ_APC_CONTEXT;
  #pragma pack(pop)

  //
  // First, map this section with read-write access.
  //

  PVOID SectionMemoryAddress = NULL;
  Status = ZwMapViewOfSection(SectionHandle,
                              ZwCurrentProcess(),
                              &SectionMemoryAddress,
                              0,
                              SectionSize,
                              NULL,
                              &SectionSize,
                              ViewUnmap,
                              0,
                              PAGE_READWRITE);

  if (!NT_SUCCESS(Status))
  {
    goto Exit;
  }

  //
  // Content of the APC routine (ApcNormalRoutine defined
  // in the shellcode above) starts at the SectionMemoryAddress.
  //

  PVOID ApcRoutine = SectionMemoryAddress;

  //
  // Context of the APC routine (the NormalContext provided
  // to the APC routine) is right behind the code of the APC
  // routine.
  //

  PINJ_APC_CONTEXT ApcContext = (PINJ_APC_CONTEXT)((PUCHAR)SectionMemoryAddress + sizeof(Shellcode));

  //
  // Copy the shellcode to the allocated memory.
  //

  RtlCopyMemory(ApcRoutine, Shellcode, sizeof(Shellcode));

  //
  // Fill the data of the ApcContext.
  //

  ApcContext->LdrLoadDll = (ULONG)(ULONG_PTR)InjectionInfo->LdrLoadDllX86;
  ApcContext->DllNameLength = GlobalData.DllPathX86.Length;
  RtlCopyMemory(ApcContext->DllNameBuffer,
                GlobalData.DllPathX86.Buffer,
                GlobalData.DllPathX86.Length);

  //
  // Unmap the section and map it again, but now
  // with read-execute (no write) access.
  //

  ZwUnmapViewOfSection(ZwCurrentProcess(), SectionMemoryAddress);

  SectionMemoryAddress = NULL;
  Status = ZwMapViewOfSection(SectionHandle,
                              ZwCurrentProcess(),
                              &SectionMemoryAddress,
                              0,
                              PAGE_SIZE,
                              NULL,
                              &SectionSize,
                              ViewUnmap,
                              0,
                              PAGE_EXECUTE_READ);

  if (!NT_SUCCESS(Status))
  {
    goto Exit;
  }

#if defined(_M_AMD64)
  //
  // PsWrapApcWow64Thread essentially assigns wow64.dll!Wow64ApcRoutine
  // to the NormalRoutine.  This Wow64ApcRoutine (which is 64-bit code)
  // in turn calls KiUserApcDispatcher (in 32-bit ntdll.dll) which finally
  // calls our provided ApcRoutine.
  //

  PsWrapApcWow64Thread(&ApcContext, &ApcRoutine);
#endif

  Status = InjpQueueApc(UserMode,
                        (PKNORMAL_ROUTINE)(ULONG_PTR)ApcRoutine,
                        ApcContext,
                        NULL,
                        NULL);

  if (!NT_SUCCESS(Status))
  {
    //
    // If injection failed for some reason, unmap the section.
    //

    ZwUnmapViewOfSection(ZwCurrentProcess(), SectionMemoryAddress);
  }

Exit:
  return Status;
}

#if defined (_M_AMD64)
NTSTATUS
NTAPI
InjpInjectX64(
  _In_ PINJ_INJECTION_INFO InjectionInfo,
  _In_ HANDLE SectionHandle,
  _In_ SIZE_T SectionSize
  )
{
  NT_ASSERT(InjectionInfo->LdrLoadDllX64);

  NTSTATUS Status;

  PVOID SectionMemoryAddress = NULL;
  Status = ZwMapViewOfSection(SectionHandle,
                              ZwCurrentProcess(),
                              &SectionMemoryAddress,
                              0,
                              PAGE_SIZE,
                              NULL,
                              &SectionSize,
                              ViewUnmap,
                              0,
                              PAGE_READWRITE);

  if (!NT_SUCCESS(Status))
  {
    goto Exit;
  }

  //
  // Create the UNICODE_STRING structure and fill out the
  // full path of the DLL.
  //

  PUNICODE_STRING DllName = (PUNICODE_STRING)(SectionMemoryAddress);
  PWCHAR DllNameBuffer = (PWCHAR)((PUCHAR)DllName + sizeof(UNICODE_STRING));

  RtlCopyMemory(DllNameBuffer,
                GlobalData.DllPathX64.Buffer,
                GlobalData.DllPathX64.MaximumLength);

  RtlInitUnicodeString(DllName, DllNameBuffer);

  Status = InjpQueueApc(UserMode,
                        (PKNORMAL_ROUTINE)(ULONG_PTR)InjectionInfo->LdrLoadDllX64,
                        NULL,     // Translates to 1st param. of LdrLoadDll (SearchPath)
                        NULL,     // Translates to 2nd param. of LdrLoadDll (DllCharacteristics)
                        DllName); // Translates to 3rd param. of LdrLoadDll (DllName)

  //
  // 4th param. of LdrLoadDll (BaseAddress) is actually an output parameter.
  //
  // When control is transferred to the KiUserApcDispatcher routine of the
  // 64-bit ntdll.dll, the RSP points to the CONTEXT structure which might
  // be eventually provided to the ZwContinue function (in case this APC
  // dispatch will be routed to the Wow64 subsystem).
  //
  // Also, the value of the RSP register is moved to the R9 register before
  // calling the KiUserCallForwarder function.  The KiUserCallForwarder
  // function actually passes this value of the R9 register down to the
  // NormalRoutine as a "hidden 4th parameter".
  //
  // Because LdrLoadDll writes to the provided address, it'll actually
  // result in overwrite of the CONTEXT.P1Home field (the first field of
  // the CONTEXT structure).
  //
  // Luckily for us, this field is only used in the very early stage of
  // the APC dispatch and can be overwritten without causing any troubles.
  //
  // For excellent explanation, see:
  // https://www.sentinelone.com/blog/deep-hooks-monitoring-native-execution-wow64-applications-part-2
  //

Exit:
  return Status;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Public functions.
//////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
InjInitialize(
  _In_ PUNICODE_STRING DllPathX86,
  _In_ PUNICODE_STRING DllPathX64,
  _In_ BOOLEAN UseWow64Injection
  )
{
#if defined(_M_IX86)
  UNREFERENCED_PARAMETER(DllPathX64);
  UNREFERENCED_PARAMETER(UseWow64Injection);
#endif

  //
  // Initialize injection info linked list.
  //

  InitializeListHead(&GlobalData.InjectionInfoListHead);

  //
  // x86-specific initialization.
  //

  GlobalData.DllPathBufferX86 = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                      DllPathX86->MaximumLength,
                                                      INJ_MEMORY_TAG);

  if (!GlobalData.DllPathBufferX86)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  GlobalData.DllPathX86.Length = DllPathX86->Length;
  GlobalData.DllPathX86.MaximumLength = DllPathX86->MaximumLength;
  GlobalData.DllPathX86.Buffer = GlobalData.DllPathBufferX86;

  RtlCopyMemory(GlobalData.DllPathBufferX86,
                DllPathX86->Buffer,
                DllPathX86->MaximumLength);

#if defined(_M_AMD64)
  //
  // x64-specific initialization.
  //

  GlobalData.DllPathBufferX64 = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                      DllPathX64->MaximumLength,
                                                      INJ_MEMORY_TAG);

  if (!GlobalData.DllPathBufferX64)
  {
    ExFreePoolWithTag(GlobalData.DllPathBufferX64, INJ_MEMORY_TAG);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  GlobalData.DllPathX64.Length = DllPathX64->Length;
  GlobalData.DllPathX64.MaximumLength = DllPathX64->MaximumLength;
  GlobalData.DllPathX64.Buffer = GlobalData.DllPathBufferX64;

  RtlCopyMemory(GlobalData.DllPathBufferX64,
                DllPathX64->Buffer,
                DllPathX64->MaximumLength);

  //
  // Default setting of the injection of Wow64 processes.
  //

  GlobalData.UseWow64Injection = UseWow64Injection;
#endif

  return STATUS_SUCCESS;
}

VOID
NTAPI
InjDestroy(
  VOID
  )
{
  PLIST_ENTRY NextEntry = GlobalData.InjectionInfoListHead.Flink;

  while (NextEntry != &GlobalData.InjectionInfoListHead)
  {
    PINJ_INJECTION_INFO InjectionInfo = CONTAINING_RECORD(NextEntry,
                                                          INJ_INJECTION_INFO,
                                                          ListEntry);
    NextEntry = NextEntry->Flink;

    ExFreePoolWithTag(InjectionInfo, INJ_MEMORY_TAG);
  }

  ExFreePoolWithTag(GlobalData.DllPathBufferX86, INJ_MEMORY_TAG);

#if defined(_M_AMD64)
  ExFreePoolWithTag(GlobalData.DllPathBufferX64, INJ_MEMORY_TAG);
#endif
}

NTSTATUS
NTAPI
InjCreateInjectionInfo(
  _In_ HANDLE ProcessId
  )
{
  PINJ_INJECTION_INFO InjectionInfo = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                            sizeof(INJ_INJECTION_INFO),
                                                            INJ_MEMORY_TAG);

  if (!InjectionInfo)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(InjectionInfo, sizeof(INJ_INJECTION_INFO));

  InjectionInfo->ProcessId = ProcessId;
  InjectionInfo->ForceUserApc = TRUE;
#if defined(_M_AMD64)
  InjectionInfo->UseWow64Injection = GlobalData.UseWow64Injection;
#endif

  InsertTailList(&GlobalData.InjectionInfoListHead, &InjectionInfo->ListEntry);

  return STATUS_SUCCESS;
}

VOID
NTAPI
InjRemoveInjectionInfo(
  _In_ HANDLE ProcessId
  )
{
  PINJ_INJECTION_INFO InjectionInfo = InjFindInjectionInfo(ProcessId);

  if (InjectionInfo)
  {
    RemoveEntryList(&InjectionInfo->ListEntry);
    ExFreePoolWithTag(InjectionInfo, INJ_MEMORY_TAG);
  }
}

PINJ_INJECTION_INFO
NTAPI
InjFindInjectionInfo(
  _In_ HANDLE ProcessId
  )
{
  PLIST_ENTRY NextEntry = GlobalData.InjectionInfoListHead.Flink;

  while (NextEntry != &GlobalData.InjectionInfoListHead)
  {
    PINJ_INJECTION_INFO InjectionInfo = CONTAINING_RECORD(NextEntry,
                                                          INJ_INJECTION_INFO,
                                                          ListEntry);

    if (InjectionInfo->ProcessId == ProcessId)
    {
      return InjectionInfo;
    }

    NextEntry = NextEntry->Flink;
  }

  return NULL;
}

BOOLEAN
NTAPI
InjCanInject(
  _In_ PINJ_INJECTION_INFO InjectionInfo
  )
{
  //
  // DLLs that need to be loaded in the Wow64 process
  // before we can safely load our DLL.
  //
  static const ULONG RequiredDllsWow64 = (INJ_SYSTEM32_NTDLL_LOADED |
                                          INJ_SYSTEM32_WOW64_LOADED |
                                          INJ_SYSTEM32_WOW64WIN_LOADED |
                                          INJ_SYSTEM32_WOW64CPU_LOADED |
                                          INJ_SYSWOW64_NTDLL_LOADED);

  //
  // DLLs that need to be loaded in the native process
  // (i.e.: 64-bit process on 64-bit Windows, 32-bit
  // process on 32-bit Windows) before we can safely
  // load our DLL.
  //
  static const ULONG RequiredDllsNative = INJ_SYSTEM32_NTDLL_LOADED;

#if defined(_M_IX86)
  const ULONG RequiredDlls = RequiredDllsNative;
#elif defined(_M_AMD64)
  const ULONG RequiredDlls = PsGetProcessWow64Process(PsGetCurrentProcess())
    ? RequiredDllsWow64
    : RequiredDllsNative;
#else
# error Unknown architecture
#endif

  return (InjectionInfo->LoadedDlls & RequiredDlls) == RequiredDlls;
}

NTSTATUS
NTAPI
InjInject(
  _In_ PINJ_INJECTION_INFO InjectionInfo
  )
{
  NTSTATUS Status;

  //
  // Create memory space for injection-specific data,
  // such as path to the to-be-injected DLL.  Memory
  // of this section will be eventually mapped to the
  // injected process.
  //
  // Note that this memory is created using sections
  // instead of ZwAllocateVirtualMemory, mainly because
  // function ZwProtectVirtualMemory is not exported
  // by ntoskrnl.exe until Windows 8.1.  In case of
  // sections, the effect of memory protection change
  // is achieved by remaping the section with different
  // protection type.
  //

  OBJECT_ATTRIBUTES ObjectAttributes;
  InitializeObjectAttributes(&ObjectAttributes,
                             NULL,
                             OBJ_KERNEL_HANDLE,
                             NULL,
                             NULL);

  HANDLE SectionHandle;
  SIZE_T SectionSize = PAGE_SIZE;
  LARGE_INTEGER MaximumSize;
  MaximumSize.QuadPart = SectionSize;
  Status = ZwCreateSection(&SectionHandle,
                           GENERIC_READ | GENERIC_WRITE,
                           &ObjectAttributes,
                           &MaximumSize,
                           PAGE_EXECUTE_READWRITE,
                           SEC_COMMIT,
                           NULL);

  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

#if defined(_M_IX86)
  Status = InjpInjectX86(InjectionInfo,
                         SectionHandle,
                         SectionSize);
#elif defined(_M_AMD64)
  if (PsGetProcessWow64Process(PsGetCurrentProcess()) &&
      InjectionInfo->UseWow64Injection)
  {
    Status = InjpInjectX86(InjectionInfo,
                           SectionHandle,
                           SectionSize);
  }
  else
  {
    Status = InjpInjectX64(InjectionInfo,
                           SectionHandle,
                           SectionSize);
  }
#endif

  ZwClose(SectionHandle);

  if (NT_SUCCESS(Status) && InjectionInfo->ForceUserApc)
  {
    //
    // Sets CurrentThread->ApcState.UserApcPending to TRUE.
    // This causes the queued user APC to be triggered immediately
    // on next transition of this thread to the user-mode.
    //
    KeTestAlertThread(UserMode);
  }

  return Status;
}

//////////////////////////////////////////////////////////////////////////
// Notify routines.
//////////////////////////////////////////////////////////////////////////

VOID
NTAPI
InjCreateProcessNotifyRoutineEx(
  _Inout_ PEPROCESS Process,
  _In_ HANDLE ProcessId,
  _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
  )
{
  UNREFERENCED_PARAMETER(Process);

  if (CreateInfo)
  {
    InjCreateInjectionInfo(ProcessId);
  }
  else
  {
    InjRemoveInjectionInfo(ProcessId);
  }
}

VOID
NTAPI
InjLoadImageNotifyRoutine(
  _In_opt_ PUNICODE_STRING FullImageName,
  _In_ HANDLE ProcessId,
  _In_ PIMAGE_INFO ImageInfo
  )
{
  //
  // Check if current process is injected.
  //

  PINJ_INJECTION_INFO InjectionInfo = InjFindInjectionInfo(ProcessId);

  if (!InjectionInfo || InjectionInfo->IsInjected)
  {
    return;
  }

  if (PsIsProtectedProcess(PsGetCurrentProcess()))
  {
    //
    // Protected processes throw code-integrity error when
    // they are injected.  Signing policy can be changed, but
    // it requires hacking with lots of internal and Windows-
    // version-specific structures.  Simply don't inject such
    // processes.
    //
    // See Blackbone project (https://github.com/DarthTon/Blackbone)
    // if you're interested how protection can be temporarily
    // disabled on such processes.  (Look for BBSetProtection).
    //

    DbgPrintEx(DPFLTR_IHVDRIVER_ID,
               DPFLTR_ERROR_LEVEL,
               "Removing protected process '%s' from watchlist...\n",
               PsGetProcessImageFileName(PsGetCurrentProcess()));

    InjRemoveInjectionInfo(ProcessId);
  }

  if (!InjCanInject(InjectionInfo))
  {
    //
    // This process is in early stage - important DLLs (such as
    // ntdll.dll and or wow64.dll in case of WoW64 process) aren't
    // properly initialized yet.  We can't inject ourselves until
    // they are.
    //
    // Check if any of the system DLLs we're interested in is being
    // currently loaded - if so, mark that information down into the
    // LoadedDlls field.
    //

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(SystemDlls); Index += 1)
    {
      if (RtlCompareUnicodeString(FullImageName, &SystemDlls[Index].DllPath, TRUE) == 0)
      {
        ULONG DllFlag = SystemDlls[Index].Flag;

        InjectionInfo->LoadedDlls |= DllFlag;

        switch (DllFlag)
        {
#if defined(_M_IX86)
          //
          // On 32-bit Windows we're interested only in the ntdll.dll
          // from the \SystemRoot\System32 directory.
          //

          case INJ_SYSTEM32_NTDLL_LOADED:
            InjectionInfo->LdrLoadDllX86 = RtlxFindExportedRoutineByName(ImageInfo->ImageBase,
                                                                         &LdrLoadDllRoutineName);
            break;
#elif defined(_M_AMD64)
          //
          // Save LdrLoadDll routine address of both native and WoW64
          // ntdll.dll.
          //

          case INJ_SYSTEM32_NTDLL_LOADED:
            InjectionInfo->LdrLoadDllX64 = RtlxFindExportedRoutineByName(ImageInfo->ImageBase,
                                                                         &LdrLoadDllRoutineName);
            break;

          case INJ_SYSWOW64_NTDLL_LOADED:
            InjectionInfo->LdrLoadDllX86 = RtlxFindExportedRoutineByName(ImageInfo->ImageBase,
                                                                         &LdrLoadDllRoutineName);
            break;
#endif
        }
      }
    }
  }
  else
  {
    //
    // All necessary DLLs are loaded - perform the injection.
    //
    // Note that injection is done via kernel-mode APC, because
    // InjInject calls ZwMapViewOfSection and MapViewOfSection
    // might be already on the callstack.  Because MapViewOfSection
    // locks the EPROCESS->AddressCreationLock, we would be risking
    // deadlock by calling InjInject directly.
    //

    InjpQueueApc(KernelMode,
                 &InjpInjectApcNormalRoutine,
                 InjectionInfo,
                 NULL,
                 NULL);

    //
    // Mark that this process is injected.
    //

    InjectionInfo->IsInjected = TRUE;
  }
}

//////////////////////////////////////////////////////////////////////////
// DriverEntry and DriverDestroy.
//////////////////////////////////////////////////////////////////////////

// VOID
// NTAPI
// DriverDestroy(
//   _In_ PDRIVER_OBJECT DriverObject
//   )
// {
//   UNREFERENCED_PARAMETER(DriverObject);
//
//   PsRemoveLoadImageNotifyRoutine(&InjLoadImageNotifyRoutine);
//   PsSetCreateProcessNotifyRoutineEx(&InjCreateProcessNotifyRoutineEx, TRUE);
//
//   InjDestroy();
// }
//
// NTSTATUS
// NTAPI
// DriverEntry(
//   _In_ PDRIVER_OBJECT DriverObject,
//   _In_ PUNICODE_STRING RegistryPath
//   )
// {
//   NTSTATUS Status;
//
//   DriverObject->DriverUnload = &DriverDestroy;
//
//   WCHAR DllPathX86Buffer[128];
//   UNICODE_STRING DllPathX86;
//   DllPathX86.Length = 0;
//   DllPathX86.MaximumLength = sizeof(DllPathX86Buffer);
//   DllPathX86.Buffer = DllPathX86Buffer;
//
//   WCHAR DllPathX64Buffer[128];
//   UNICODE_STRING DllPathX64;
//   DllPathX64.Length = 0;
//   DllPathX64.MaximumLength = sizeof(DllPathX64Buffer);
//   DllPathX64.Buffer = DllPathX64Buffer;
//
//   Status = InjpInitializeDllPaths(RegistryPath,
//                                   &DllPathX86,
//                                   &DllPathX64);
//
//   if (!NT_SUCCESS(Status))
//   {
//     return Status;
//   }
//
//   //
//   // #define INJ_CUSTOM_PATH L"C:\\Users\\John\\Desktop\\"
//   //   UNICODE_STRING DllPathX86 = RTL_CONSTANT_STRING(INJ_CUSTOM_PATH L"keinject\\x86\\Debug\\injdll.dll");
//   //   UNICODE_STRING DllPathX64 = RTL_CONSTANT_STRING(INJ_CUSTOM_PATH L"keinject\\x64\\Debug\\injdll.dll");
//   //
//
//   BOOLEAN UseWow64Injection = FALSE;
//   Status = InjInitialize(&DllPathX86,
//                          &DllPathX64,
//                          UseWow64Injection);
//
//   if (!NT_SUCCESS(Status))
//   {
//     return Status;
//   }
//
//   //
//   // Install CreateProcess and LoadImage notification routines.
//   //
//
//   Status = PsSetCreateProcessNotifyRoutineEx(&InjCreateProcessNotifyRoutineEx, FALSE);
//
//   if (!NT_SUCCESS(Status))
//   {
//     return Status;
//   }
//
//   Status = PsSetLoadImageNotifyRoutine(&InjLoadImageNotifyRoutine);
//
//   if (!NT_SUCCESS(Status))
//   {
//     PsSetCreateProcessNotifyRoutineEx(&InjCreateProcessNotifyRoutineEx, TRUE);
//     return Status;
//   }
//
//   return STATUS_SUCCESS;
// }
