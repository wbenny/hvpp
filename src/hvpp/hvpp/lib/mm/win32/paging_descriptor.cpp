#include "hvpp/ia32/memory.h"

#include <ntddk.h>
#include <ntimage.h>

//////////////////////////////////////////////////////////////////////////
// Taken from windbgexts.h (Windows SDK, "um" folder)
//////////////////////////////////////////////////////////////////////////

#define KDBG_TAG    'GBDK'

//
// This structure is used by the debugger for all targets
// It is the same size as DBGKD_DATA_HEADER on all systems
//
typedef struct _DBGKD_DEBUG_DATA_HEADER64 {

    //
    // Link to other blocks
    //

    LIST_ENTRY64 List;

    //
    // This is a unique tag to identify the owner of the block.
    // If your component only uses one pool tag, use it for this, too.
    //

    ULONG           OwnerTag;

    //
    // This must be initialized to the size of the data block,
    // including this structure.
    //

    ULONG           Size;

} DBGKD_DEBUG_DATA_HEADER64, *PDBGKD_DEBUG_DATA_HEADER64;


//
// This structure is the same size on all systems.  The only field
// which must be translated by the debugger is Header.List.
//

//
// DO NOT ADD OR REMOVE FIELDS FROM THE MIDDLE OF THIS STRUCTURE!!!
//
// If you remove a field, replace it with an "unused" placeholder.
// Do not reuse fields until there has been enough time for old debuggers
// and extensions to age out.
//
typedef struct _KDDEBUGGER_DATA64 {

    DBGKD_DEBUG_DATA_HEADER64 Header;

    //
    // Base address of kernel image
    //

    ULONG64   KernBase;

    //
    // DbgBreakPointWithStatus is a function which takes an argument
    // and hits a breakpoint.  This field contains the address of the
    // breakpoint instruction.  When the debugger sees a breakpoint
    // at this address, it may retrieve the argument from the first
    // argument register, or on x86 the eax register.
    //

    ULONG64   BreakpointWithStatus;       // address of breakpoint

    //
    // Address of the saved context record during a bugcheck
    //
    // N.B. This is an automatic in KeBugcheckEx's frame, and
    // is only valid after a bugcheck.
    //

    ULONG64   SavedContext;

    //
    // help for walking stacks with user callbacks:
    //

    //
    // The address of the thread structure is provided in the
    // WAIT_STATE_CHANGE packet.  This is the offset from the base of
    // the thread structure to the pointer to the kernel stack frame
    // for the currently active usermode callback.
    //

    USHORT  ThCallbackStack;            // offset in thread data

    //
    // these values are offsets into that frame:
    //

    USHORT  NextCallback;               // saved pointer to next callback frame
    USHORT  FramePointer;               // saved frame pointer

    //
    // pad to a quad boundary
    //
    USHORT  PaeEnabled:1;

    //
    // Address of the kernel callout routine.
    //

    ULONG64   KiCallUserMode;             // kernel routine

    //
    // Address of the usermode entry point for callbacks.
    //

    ULONG64   KeUserCallbackDispatcher;   // address in ntdll


    //
    // Addresses of various kernel data structures and lists
    // that are of interest to the kernel debugger.
    //

    ULONG64   PsLoadedModuleList;
    ULONG64   PsActiveProcessHead;
    ULONG64   PspCidTable;

    ULONG64   ExpSystemResourcesList;
    ULONG64   ExpPagedPoolDescriptor;
    ULONG64   ExpNumberOfPagedPools;

    ULONG64   KeTimeIncrement;
    ULONG64   KeBugCheckCallbackListHead;
    ULONG64   KiBugcheckData;

    ULONG64   IopErrorLogListHead;

    ULONG64   ObpRootDirectoryObject;
    ULONG64   ObpTypeObjectType;

    ULONG64   MmSystemCacheStart;
    ULONG64   MmSystemCacheEnd;
    ULONG64   MmSystemCacheWs;

    ULONG64   MmPfnDatabase;
    ULONG64   MmSystemPtesStart;
    ULONG64   MmSystemPtesEnd;
    ULONG64   MmSubsectionBase;
    ULONG64   MmNumberOfPagingFiles;

    ULONG64   MmLowestPhysicalPage;
    ULONG64   MmHighestPhysicalPage;
    ULONG64   MmNumberOfPhysicalPages;

    ULONG64   MmMaximumNonPagedPoolInBytes;
    ULONG64   MmNonPagedSystemStart;
    ULONG64   MmNonPagedPoolStart;
    ULONG64   MmNonPagedPoolEnd;

    ULONG64   MmPagedPoolStart;
    ULONG64   MmPagedPoolEnd;
    ULONG64   MmPagedPoolInformation;
    ULONG64   MmPageSize;

    ULONG64   MmSizeOfPagedPoolInBytes;

    ULONG64   MmTotalCommitLimit;
    ULONG64   MmTotalCommittedPages;
    ULONG64   MmSharedCommit;
    ULONG64   MmDriverCommit;
    ULONG64   MmProcessCommit;
    ULONG64   MmPagedPoolCommit;
    ULONG64   MmExtendedCommit;

    ULONG64   MmZeroedPageListHead;
    ULONG64   MmFreePageListHead;
    ULONG64   MmStandbyPageListHead;
    ULONG64   MmModifiedPageListHead;
    ULONG64   MmModifiedNoWritePageListHead;
    ULONG64   MmAvailablePages;
    ULONG64   MmResidentAvailablePages;

    ULONG64   PoolTrackTable;
    ULONG64   NonPagedPoolDescriptor;

    ULONG64   MmHighestUserAddress;
    ULONG64   MmSystemRangeStart;
    ULONG64   MmUserProbeAddress;

    ULONG64   KdPrintCircularBuffer;
    ULONG64   KdPrintCircularBufferEnd;
    ULONG64   KdPrintWritePointer;
    ULONG64   KdPrintRolloverCount;

    ULONG64   MmLoadedUserImageList;

    // NT 5.1 Addition

    ULONG64   NtBuildLab;
    ULONG64   KiNormalSystemCall;

    // NT 5.0 hotfix addition

    ULONG64   KiProcessorBlock;
    ULONG64   MmUnloadedDrivers;
    ULONG64   MmLastUnloadedDriver;
    ULONG64   MmTriageActionTaken;
    ULONG64   MmSpecialPoolTag;
    ULONG64   KernelVerifier;
    ULONG64   MmVerifierData;
    ULONG64   MmAllocatedNonPagedPool;
    ULONG64   MmPeakCommitment;
    ULONG64   MmTotalCommitLimitMaximum;
    ULONG64   CmNtCSDVersion;

    // NT 5.1 Addition

    ULONG64   MmPhysicalMemoryBlock;
    ULONG64   MmSessionBase;
    ULONG64   MmSessionSize;
    ULONG64   MmSystemParentTablePage;

    // Server 2003 addition

    ULONG64   MmVirtualTranslationBase;

    USHORT    OffsetKThreadNextProcessor;
    USHORT    OffsetKThreadTeb;
    USHORT    OffsetKThreadKernelStack;
    USHORT    OffsetKThreadInitialStack;

    USHORT    OffsetKThreadApcProcess;
    USHORT    OffsetKThreadState;
    USHORT    OffsetKThreadBStore;
    USHORT    OffsetKThreadBStoreLimit;

    USHORT    SizeEProcess;
    USHORT    OffsetEprocessPeb;
    USHORT    OffsetEprocessParentCID;
    USHORT    OffsetEprocessDirectoryTableBase;

    USHORT    SizePrcb;
    USHORT    OffsetPrcbDpcRoutine;
    USHORT    OffsetPrcbCurrentThread;
    USHORT    OffsetPrcbMhz;

    USHORT    OffsetPrcbCpuType;
    USHORT    OffsetPrcbVendorString;
    USHORT    OffsetPrcbProcStateContext;
    USHORT    OffsetPrcbNumber;

    USHORT    SizeEThread;

    UCHAR     L1tfHighPhysicalBitIndex;  // Windows 10 19H1 Addition
    UCHAR     L1tfSwizzleBitIndex;       // Windows 10 19H1 Addition

    ULONG     Padding0;

    ULONG64   KdPrintCircularBufferPtr;
    ULONG64   KdPrintBufferSize;

    ULONG64   KeLoaderBlock;

    USHORT    SizePcr;
    USHORT    OffsetPcrSelfPcr;
    USHORT    OffsetPcrCurrentPrcb;
    USHORT    OffsetPcrContainedPrcb;

    USHORT    OffsetPcrInitialBStore;
    USHORT    OffsetPcrBStoreLimit;
    USHORT    OffsetPcrInitialStack;
    USHORT    OffsetPcrStackLimit;

    USHORT    OffsetPrcbPcrPage;
    USHORT    OffsetPrcbProcStateSpecialReg;
    USHORT    GdtR0Code;
    USHORT    GdtR0Data;

    USHORT    GdtR0Pcr;
    USHORT    GdtR3Code;
    USHORT    GdtR3Data;
    USHORT    GdtR3Teb;

    USHORT    GdtLdt;
    USHORT    GdtTss;
    USHORT    Gdt64R3CmCode;
    USHORT    Gdt64R3CmTeb;

    ULONG64   IopNumTriageDumpDataBlocks;
    ULONG64   IopTriageDumpDataBlocks;

    // Longhorn addition

    ULONG64   VfCrashDataBlock;
    ULONG64   MmBadPagesDetected;
    ULONG64   MmZeroedPageSingleBitErrorsDetected;

    // Windows 7 addition

    ULONG64   EtwpDebuggerData;
    USHORT    OffsetPrcbContext;

    // Windows 8 addition

    USHORT    OffsetPrcbMaxBreakpoints;
    USHORT    OffsetPrcbMaxWatchpoints;

    ULONG     OffsetKThreadStackLimit;
    ULONG     OffsetKThreadStackBase;
    ULONG     OffsetKThreadQueueListEntry;
    ULONG     OffsetEThreadIrpList;

    USHORT    OffsetPrcbIdleThread;
    USHORT    OffsetPrcbNormalDpcState;
    USHORT    OffsetPrcbDpcStack;
    USHORT    OffsetPrcbIsrStack;

    USHORT    SizeKDPC_STACK_FRAME;

    // Windows 8.1 Addition

    USHORT    OffsetKPriQueueThreadListHead;
    USHORT    OffsetKThreadWaitReason;

    // Windows 10 RS1 Addition

    USHORT    Padding1;
    ULONG64   PteBase;

    // Windows 10 RS5 Addition

    ULONG64   RetpolineStubFunctionTable;
    ULONG     RetpolineStubFunctionTableSize;
    ULONG     RetpolineStubOffset;
    ULONG     RetpolineStubSize;

} KDDEBUGGER_DATA64, *PKDDEBUGGER_DATA64;

//////////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////////

EXTERN_C
NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
  _In_ PVOID BaseOfImage
  );

EXTERN_C
NTSYSAPI
PVOID
NTAPI
RtlPcToFileHeader(
  _In_ PVOID PcValue,
  _Out_ PVOID* BaseOfImage
  );

_Success_(return != NULL)
PIMAGE_SECTION_HEADER
NTAPI
RtlxSectionTableFromVirtualAddress(
  _In_ PIMAGE_NT_HEADERS NtHeaders,
  _In_ ULONG Address
  )
{
  //
  // RtlSectionTableFromVirtualAddress
  //

  PIMAGE_SECTION_HEADER NtSection = IMAGE_FIRST_SECTION(NtHeaders);

  for (ULONG Index = 0; Index < NtHeaders->FileHeader.NumberOfSections; Index++)
  {
    if (
      (ULONG)Address >= NtSection->VirtualAddress &&
      (ULONG)Address  < NtSection->VirtualAddress + NtSection->SizeOfRawData
      )
    {
      return NtSection;
    }

    NtSection++;
  }

  return NULL;
}

_Success_(return != NULL)
PIMAGE_SECTION_HEADER
NTAPI
RtlxSectionTableFromSectionName(
  _In_ PIMAGE_NT_HEADERS NtHeaders,
  _In_ PCHAR SectionName
  )
{
  //
  // RtlSectionTableFromSectionName
  //

  PIMAGE_SECTION_HEADER NtSection = IMAGE_FIRST_SECTION(NtHeaders);

  for (ULONG Index = 0; Index < NtHeaders->FileHeader.NumberOfSections; Index++)
  {
    if (!strcmp((PCHAR)NtSection->Name, SectionName))
    {
      return NtSection;
    }

    NtSection++;
  }

  return NULL;
}

PKDDEBUGGER_DATA64
NTAPI
FindKdDebuggerDataBlock()
{
  //
  // Find image base address of ntoskrnl.exe.
  //
  PVOID NtoskrnlBase;
  RtlPcToFileHeader(&RtlPcToFileHeader, &NtoskrnlBase);

  if (!NtoskrnlBase)
  {
    return NULL;
  }

  //
  // Find NT headers.
  //
  PIMAGE_NT_HEADERS NtHeaders;
  NtHeaders = RtlImageNtHeader(NtoskrnlBase);

  if (!NtHeaders)
  {
    return NULL;
  }

  //
  // Find ".data" section header.
  // "KdDebuggerDataBlock" is located in this section.
  //
  PIMAGE_SECTION_HEADER SectionHeader;
  SectionHeader = RtlxSectionTableFromSectionName(NtHeaders, (PCHAR)".data");

  if (!SectionHeader)
  {
    return NULL;
  }

  //
  // Check for "KDBG" signature in the ".data" section.
  //
  PUCHAR VaBegin = (PUCHAR)(NtoskrnlBase) + SectionHeader->VirtualAddress;
  PUCHAR VaEnd   = VaBegin + SectionHeader->SizeOfRawData - sizeof(ULONG);

  do if (*(PULONG)(VaBegin) == KDBG_TAG)
    break;
  while (++VaBegin < VaEnd);

  if (VaBegin >= VaEnd)
  {
    return NULL;
  }

  //
  // "KDBG" signature is in-fact INSIDE of the KDDEBUGGER_DATA64.
  // This will find the real start of the KDDEBUGGER_DATA64 structure.
  //
  PKDDEBUGGER_DATA64 KdDebuggerDataBlock;
  KdDebuggerDataBlock = CONTAINING_RECORD(VaBegin, KDDEBUGGER_DATA64, Header.OwnerTag);

  return KdDebuggerDataBlock;
}

UINT64
NTAPI
FindSystemDirectoryTableBase()
{
  struct NT_KPROCESS
  {
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    ULONG_PTR DirectoryTableBase;
    UCHAR Data[1];
  };

  //
  // Return CR3 of the system process.
  //

  NT_KPROCESS* SystemProcess = reinterpret_cast<NT_KPROCESS*>(PsInitialSystemProcess);
  return SystemProcess->DirectoryTableBase;
}

//////////////////////////////////////////////////////////////////////////
// namespace mm::detail
//////////////////////////////////////////////////////////////////////////

#define PXE_BASE          0xFFFFF6FB7DBED000UI64
#define PPE_BASE          0xFFFFF6FB7DA00000UI64
#define PDE_BASE          0xFFFFF6FB40000000UI64
#define PTE_BASE          0xFFFFF68000000000UI64

#define PTI_SHIFT 12
#define PDI_SHIFT 21
#define PPI_SHIFT 30
#define PXI_SHIFT 39

using namespace ia32;

namespace mm::detail
{
  void check_paging(uint64_t* pml4_base, uint64_t* pdpt_base, uint64_t* pd_base, uint64_t* pt_base) noexcept
  {
    RTL_OSVERSIONINFOW OsVersionInfo;
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    //
    // According to MSDN, this function never fails.
    //
    RtlGetVersion(&OsVersionInfo);

    //
    // Anything less than Windows 10 RS1 should have static page table addresses.
    //
    if (OsVersionInfo.dwMajorVersion < 10 || OsVersionInfo.dwBuildNumber < 14316)
    {
      *pml4_base = PXE_BASE;
      *pdpt_base = PPE_BASE;
      *pd_base   = PDE_BASE;
      *pt_base   = PTE_BASE;
      return;
    }

    PKDDEBUGGER_DATA64 KdDebuggerDataBlock = FindKdDebuggerDataBlock();

    if (!KdDebuggerDataBlock)
    {
      *pml4_base = 0;
      *pdpt_base = 0;
      *pd_base   = 0;
      *pt_base   = 0;
      return;
    }

    ULONG64 Index = (KdDebuggerDataBlock->PteBase >> PXI_SHIFT) & 0x1FF;

    ULONG64 PteBase = KdDebuggerDataBlock->PteBase;
    ULONG64 PdeBase = PteBase | (Index << PPI_SHIFT);
    ULONG64 PpeBase = PdeBase | (Index << PDI_SHIFT);
    ULONG64 PxeBase = PpeBase | (Index << PTI_SHIFT);

    *pml4_base = PxeBase;
    *pdpt_base = PpeBase;
    *pd_base   = PdeBase;
    *pt_base   = PteBase;
  }

  void check_system_cr3(uint64_t* system_cr3) noexcept
  {
    UINT64 SystemDirectoryTableBase = FindSystemDirectoryTableBase();

    *system_cr3 = SystemDirectoryTableBase;
  }
}
