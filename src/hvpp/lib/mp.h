#pragma once
#include <cstdint>
#include <ntddk.h>

extern "C" {

NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
KeGenericCallDpc (
    _In_ PKDEFERRED_ROUTINE Routine,
    _In_opt_ PVOID Context
    );

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
KeSignalCallDpcDone (
    _In_ PVOID SystemArgument1
    );

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
LOGICAL
KeSignalCallDpcSynchronize (
    _In_ PVOID SystemArgument2
    );

}

namespace mp {

template <typename T>
inline void ipi_call(T* instance, void (T::*member_function)() noexcept) noexcept
{
  struct ipi_ctx
  {
    T* instance;
    void (T::*member_function)() noexcept;
  } context {
    instance, member_function
  };

  KeIpiGenericCall([](ULONG_PTR context) noexcept -> ULONG_PTR {
    auto ipi_context = (ipi_ctx*)context;
    auto instance = ipi_context->instance;
    auto member_function = ipi_context->member_function;

    (instance->*member_function)();

    return 0;
  }, (ULONG_PTR)&context);
}

template <typename T>
inline void dpc_call(T* instance, void (T::*member_function)() noexcept) noexcept
{
  struct dpc_ctx
  {
    T* instance;
    void (T::*member_function)() noexcept;
  } context {
    instance, member_function
  };

  KeGenericCallDpc([](PRKDPC Dpc, PVOID Context, PVOID SystemArgument1, PVOID SystemArgument2) noexcept {
    UNREFERENCED_PARAMETER(Dpc);

    auto dpc_context = (dpc_ctx*)Context;
    auto instance = dpc_context->instance;
    auto member_function = dpc_context->member_function;

    (instance->*member_function)();

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
  }, &context);
}

inline uint32_t cpu_index() noexcept
{
  return KeGetCurrentProcessorNumberEx(NULL);
}

inline void sleep(uint32_t milliseconds) noexcept
{
  LARGE_INTEGER interval;
  interval.QuadPart = -(10000ll * milliseconds);

  KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

}
