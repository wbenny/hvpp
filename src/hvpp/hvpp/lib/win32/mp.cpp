#include "../mp.h"

#include <cstdint>

#include <ntddk.h>

namespace mp::detail
{
  uint32_t cpu_count() noexcept
  {
    return KeQueryActiveProcessorCountEx(0);
  }

  uint32_t cpu_index() noexcept
  {
    return KeGetCurrentProcessorNumberEx(NULL);
  }

  void sleep(uint32_t milliseconds) noexcept
  {
    LARGE_INTEGER interval;
    interval.QuadPart = -(10000ll * milliseconds);

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
  }

  void ipi_call(void(*callback)(void*), void* context) noexcept
  {
    struct ipi_ctx
    {
      void* context;
      void(*callback)(void*);
    } ipi_context {
      context,
      callback
    };

    KeIpiGenericCall([](ULONG_PTR Context) noexcept -> ULONG_PTR {
      //
      // Note that the function is called with IRQL at IPI_LEVEL.
      // Keep in mind that this effectively forbids us to call most of the kernel
      // functions.
      //
    
      auto ipi_context = reinterpret_cast<ipi_ctx*>(Context);
      auto context = ipi_context->context;
      auto callback = ipi_context->callback;

      callback(context);
      return 0;
    }, (ULONG_PTR)&ipi_context);
  }
}
