#ifndef _NTDLL_H
#define _NTDLL_H

// This header file provides access to NT APIs.

// Definitions are annotated to indicate their source. If a definition is not annotated, it has been
// retrieved from an official Microsoft source (NT headers, DDK headers, winnt.h).

// * "winbase" indicates that a definition has been reconstructed from a Win32-ized NT definition in
//   winbase.h.
// * "rev" indicates that a definition has been reverse-engineered.
// * "dbg" indicates that a definition has been obtained from a debug message or assertion in a
//   checked build of the kernel or file.

// Reliability:
// 1. No annotation.
// 2. dbg.
// 3. symbols, private. Types may be incorrect.
// 4. winbase. Names and types may be incorrect.
// 5. rev.

// Mode
#define NTDLL_MODE_KERNEL 0
#define NTDLL_MODE_USER 1

// Version
#define NTDLL_WIN2K 50
#define NTDLL_WINXP 51
#define NTDLL_WS03 52
#define NTDLL_VISTA 60
#define NTDLL_WIN7 61
#define NTDLL_WIN8 62
#define NTDLL_WINBLUE 63
#define NTDLL_THRESHOLD 100
#define NTDLL_THRESHOLD2 101
#define NTDLL_REDSTONE 102
#define NTDLL_REDSTONE2 103
#define NTDLL_REDSTONE3 104
#define NTDLL_REDSTONE4 105

#ifndef NTDLL_MODE
#define NTDLL_MODE NTDLL_MODE_USER
#endif

#ifndef NTDLL_VERSION
#define NTDLL_VERSION NTDLL_WIN7
#endif

// Options

//#define NTDLL_NO_INLINE_INIT_STRING

#ifdef __cplusplus
extern "C" {
#endif

#if (NTDLL_MODE != NTDLL_MODE_KERNEL)
#include <ntdll_ntdef.h>
#include <ntnls.h>
#include <ntkeapi.h>
#endif

#include <ntldr.h>
#include <ntexapi.h>

#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>

#if (NTDLL_MODE != NTDLL_MODE_KERNEL)
#include <cfg.h>
#include <ntdbg.h>
#include <ntetw.h>
#include <ntioapi.h>
#include <ntlpcapi.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#endif

#if (NTDLL_MODE != NTDLL_MODE_KERNEL)

#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntxcapi.h>

#include <ntwow64.h>

#include <ntlsa.h>
#include <ntsam.h>

#include <ntmisc.h>

#include <ntzwapi.h>

#endif

#ifdef __cplusplus
}
#endif

#endif
