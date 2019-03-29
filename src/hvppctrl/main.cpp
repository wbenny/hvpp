#include <cstdio>
#include <cstdint>

#include <windows.h>

#include "ia32/asm.h"
#include "lib/mp.h"
#include "detours/detours.h"
#include "udis86/udis86.h"

#include "../hvpp/hvpp/lib/ioctl.h"

using ioctl_enable_io_debugbreak_t = ioctl_read_write_t<1, sizeof(uint16_t)>;

#define PAGE_SIZE       4096
#define PAGE_ALIGN(Va)  ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

// #define ia32_asm_vmx_vmcall(...)

static int HookCallCount = 0;

using pfnZwClose = NTSTATUS (*)(_In_ HANDLE Handle);

DECLSPEC_NOINLINE
NTSTATUS
Hook_ZwClose(
  _In_ HANDLE Handle
  )
{
  HookCallCount += 1;
  return 0;
}

void TestCpuid()
{
  //
  // See vmexit_custom_handler::handle_execute_cpuid().
  //
  uint32_t CpuInfo[4];
  ia32_asm_cpuid(CpuInfo, 'ppvh');

  printf("CPUID: '%s'\n\n", (const char*)CpuInfo);
}

void TestHook()
{
  pfnZwClose ZwCloseFn = (pfnZwClose)GetProcAddress(LoadLibraryA("ntdll.dll"), "ZwClose");

  //
  // Save pointer to the function from which we want to hide the hook.
  // Also, since EPTs have 4kb (PAGE_SIZE) granularity, it will allow
  // us to switch just whole pages - not only few bytes.  Therefore
  // we'll store pointer which is aligned to the page boundary.
  //
  PVOID OriginalFunction        = (PVOID)ZwCloseFn;
  PVOID OriginalFunctionAligned = PAGE_ALIGN(OriginalFunction);

  //
  // Allocate memory where we'll copy the original content of the page
  // we're about to hook.  We'll allocate size of 2 pages, since malloc()
  // doesn't guarantee the return address is page aligned.  We have to
  // do it ourselves.
  //
  PVOID OriginalFunctionBackup  = malloc(PAGE_SIZE * 2);
  PVOID OriginalFunctionBackupAligned = PAGE_ALIGN((ULONG_PTR)OriginalFunctionBackup + PAGE_SIZE);
  memcpy(OriginalFunctionBackupAligned, OriginalFunctionAligned, PAGE_SIZE);

  //
  // Store pointer to the function which will be called instead of
  // the original one.
  //
  PVOID HookedFunction          = (PVOID)&Hook_ZwClose;

  //
  // Print useful information.
  //
  printf("OriginalFunction        : 0x%p\n", OriginalFunction);
  printf("OriginalFunctionAligned : 0x%p\n", OriginalFunctionAligned);
  printf("OriginalPage            : 0x%p\n", OriginalFunctionBackup);
  printf("OriginalPageAligned     : 0x%p\n", OriginalFunctionBackupAligned);
  printf("HookedFunction          : 0x%p\n", HookedFunction);
  printf("\n");

  //
  // Define locally functions for disassembling, (un)hooking and
  // (un)hiding.  Disassembling is useful to check what's in the
  // memory if we request read access.
  //
  auto Disassemble = [](void* Address)
  {
    ud_t u;
    ud_init(&u);

    ud_set_input_buffer(&u, (uint8_t*)Address, 16);
    ud_set_mode(&u, 64);
    ud_set_syntax(&u, UD_SYN_INTEL);

    while (ud_disassemble(&u))
    {
      printf("\t%s\n", ud_insn_asm(&u));
    }
  };

  auto Hook = [](void** OriginalFunction, void* HookedFunction)
  {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(OriginalFunction, HookedFunction);
    DetourTransactionCommit();
  };

  auto Unhook = [](void** OriginalFunction, void* HookedFunction)
  {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(OriginalFunction, HookedFunction);
    DetourTransactionCommit();
  };

  auto Hide = [](void* PageRead, void* PageExecute)
  {
    struct CONTEXT { void* PageRead; void* PageExecute; } Context { PageRead, PageExecute };
    ForEachLogicalCore([](void* ContextPtr) {
      CONTEXT* Context = (CONTEXT*)ContextPtr;
      ia32_asm_vmx_vmcall(0xc1, (uint64_t)Context->PageRead, (uint64_t)Context->PageExecute, 0);
    }, &Context);
  };

  auto Unhide = []()
  {
    ForEachLogicalCore([](void*) { ia32_asm_vmx_vmcall(0xc2, 0, 0, 0); }, nullptr);
  };

  //
  // Lock the pages in the RAM.  Hopefully, they won't be swapped out.
  //

  VirtualLock(OriginalFunctionAligned, PAGE_SIZE);
  VirtualLock(OriginalFunctionBackupAligned, PAGE_SIZE);

  //
  // No hooks, no hiding - call the original function.
  //

  printf("Original:\n");
  Disassemble(ZwCloseFn);
  ZwCloseFn(NULL);
  printf("HookCallCount = %i (expected: 0)\n\n", HookCallCount);
  printf("\n");

  //
  // Hook the function and call it.  If the hooking was successful,
  // HookCallCount should be already incremented.
  //

  printf("Hook:\n");
  Hook(&OriginalFunction, HookedFunction);
  Disassemble(ZwCloseFn);
  ZwCloseFn(NULL);
  printf("HookCallCount = %i (expected: 1)\n\n", HookCallCount);
  printf("\n");

  //
  // Hide the hook and instruct the hypervisor to return memory of
  // "OriginalFunctionBackupAligned" when read    is requested and
  // "OriginalFunctionAligned"       when execute is requested.
  //
  // The output of Disassemble() should be same as when we called it
  // without any hooks, but the call of the function should be detoured
  // to our hook function - therefore HookCallCount should be incremented
  // again.
  //

  printf("Hide:\n");
  Hide(OriginalFunctionBackupAligned, OriginalFunctionAligned);
  Disassemble(ZwCloseFn);
  ZwCloseFn(NULL);
  printf("HookCallCount = %i (expected: 2)\n\n", HookCallCount);
  printf("\n");

  //
  // Unhide the hook and instruct the hypervisor to return original
  // pages for both read and execute.  The output of Disassemble()
  // should be same as when we called it after hooking and the call
  // should be still detoured to our hook function.  HookCallCount
  // should be incremented again.
  //

  printf("Unhide:\n");
  Unhide();
  Disassemble(ZwCloseFn);
  ZwCloseFn(NULL);
  printf("HookCallCount = %i (expected: 3)\n\n", HookCallCount);
  printf("\n");

  //
  // Unhook the function.  Disassemble() should be same as when we
  // called it without any hooks and the call of the function should
  // call the original function - therefore HookCallCount shouldn't
  // be incremented this time.
  //

  printf("Unhook:\n");
  Unhook(&OriginalFunction, HookedFunction);
  Disassemble(ZwCloseFn);
  ZwCloseFn(NULL);
  printf("HookCallCount = %i (expected: 3)\n\n", HookCallCount);
  printf("\n");

  //
  // Finally, unlock the pages and free them.
  //

  VirtualUnlock(OriginalFunctionAligned, PAGE_SIZE);
  VirtualUnlock(OriginalFunctionBackupAligned, PAGE_SIZE);
  free(OriginalFunctionBackup);
}

void TestIoControl()
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

  //
  // Issue IOCTL call to the driver.
  // When kernel debugger is attached, this IOCTL will instruct
  // the hypervisor to set one-time breakpoint when IN/OUT
  // instruction from/to port 0x64 (keyboard I/O port) is executed.
  //
  // See hvpp/device_custom.cpp.
  //

  UINT16 IoPort = 0x64;
  DWORD BytesReturned;
  DeviceIoControl(DeviceHandle,
                  ioctl_enable_io_debugbreak_t::code,
                  &IoPort,
                  sizeof(IoPort),
                  &IoPort,
                  sizeof(IoPort),
                  &BytesReturned,
                  NULL);

  CloseHandle(DeviceHandle);

  //
  // Return value should be 0x1337 if the kernel debugger
  // is attached, 0xCAFE otherwise.
  //
  printf("IOCTL return value: 0x%04x (size: %u)\n", IoPort, BytesReturned);
}

int main()
{
  TestCpuid();
  TestHook();
  TestIoControl();

  return 0;
}

