#pragma once

//
// Maximum number of CPUs.
//
#define HVPP_MAX_CPU  256

//
// Disable logging (DbgPrintEx) and/or ETW logging.
//

// #define HVPP_DISABLE_LOG

// #define HVPP_DISABLE_TRACELOG

//
// Disable asserts (hvpp_assert()).
//

// #define HVPP_DISABLE_ASSERT

//
// Uncomment this if you plan to intercept I/O ports 0x5658/0x5659
// in VMWare and you don't want the VMWare Tools to crash.
//
#define HVPP_ENABLE_VMWARE_WORKAROUND
