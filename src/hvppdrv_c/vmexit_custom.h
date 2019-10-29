#pragma once
#include <ntddk.h>
#include <hvpp/hvpp.h>

NTSTATUS
NTAPI
HvppSetup(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

VOID
NTAPI
HvppTeardown(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

VOID
NTAPI
HvppHandleExecuteCpuid(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

VOID
NTAPI
HvppHandleExecuteVmcall(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

VOID
NTAPI
HvppHandleEptViolation(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );
