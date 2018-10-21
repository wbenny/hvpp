#define _NO_CRT_STDIO_INLINE

#include "trace.h"

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>

//
// GUID:
//   {a4b4ba50-a667-43f5-919b-1e52a6d69bd5}
//

GUID ProviderGuid = {
  0xa4b4ba50, 0xa667, 0x43f5, { 0x91, 0x9b, 0x1e, 0x52, 0xa6, 0xd6, 0x9b, 0xd5 }
};

REGHANDLE ProviderHandle;

NTSTATUS
NTAPI
TraceInitialize(
  VOID
  )
{
  NTSTATUS Status;

  Status = EtwRegister(&ProviderGuid,
                       NULL,
                       NULL,
                       &ProviderHandle);

  return Status;
}

VOID
NTAPI
TraceDestroy(
  VOID
  )
{
  EtwUnregister(ProviderHandle);
}

VOID
NTAPI
Trace(
  _In_ PWCHAR Message
  )
{
  EtwWriteString(ProviderHandle, 0, 0, NULL, Message);
}

VOID
TraceFormat(
  _In_ const char* Format,
  ...
  )
{
  CHAR MessageAnsi[512];
  WCHAR MessageUnicode[512];

  va_list Args;
  va_start(Args, Format);
  vsprintf_s(MessageAnsi, sizeof(MessageAnsi), Format, Args);
  va_end(Args);

  NTSTATUS Status;

  ANSI_STRING MessageAnsiString;
  RtlInitAnsiString(&MessageAnsiString, MessageAnsi);

  UNICODE_STRING MessageUnicodeString;
  MessageUnicodeString.Length = 0;
  MessageUnicodeString.MaximumLength = sizeof(MessageUnicode);
  MessageUnicodeString.Buffer = MessageUnicode;
  Status = RtlAnsiStringToUnicodeString(&MessageUnicodeString, &MessageAnsiString, FALSE);

  if (NT_SUCCESS(Status))
  {
    MessageUnicode[MessageUnicodeString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    Trace(MessageUnicode);
  }
}
