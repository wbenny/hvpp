#pragma once

//
// Uncomment this if you want to subvert just one CPU (with ID 0).
// This can be helpful for debugging purposes.
//
// #define HVPP_SINGLE_VCPU

//
// Uncomment this if you plan to intercept I/O ports 0x5658/0x5659
// in VMWare and you don't want the VMWare Tools to crash.
//
#define HVPP_ENABLE_VMWARE_WORKAROUND

//
// Use vmexit_stats_handler instead of vmexit_handler as a base class
// for custom_vmexit_handler.
//
#define HVPP_ENABLE_STATS
