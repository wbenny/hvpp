#pragma once

#if 0
#include <ntddk.h>
#endif

#pragma warning(push)
#pragma warning(disable: 4214)

#ifdef __cplusplus
extern "C" {
#endif

#pragma region ia32

#pragma region arch.h

typedef enum _VCPU_CONTEXT_REG
{
  ContextRegRax                                               =  0,
  ContextRegRcx                                               =  1,
  ContextRegRdx                                               =  2,
  ContextRegRbx                                               =  3,
  ContextRegRsp                                               =  4,
  ContextRegRbp                                               =  5,
  ContextRegRsi                                               =  6,
  ContextRegRdi                                               =  7,
  ContextRegR8                                                =  8,
  ContextRegR9                                                =  9,
  ContextRegR10                                               = 10,
  ContextRegR11                                               = 11,
  ContextRegR12                                               = 12,
  ContextRegR13                                               = 13,
  ContextRegR14                                               = 14,
  ContextRegR15                                               = 15,

  ContextRegMin                                               =  0,
  ContextRegMax                                               = 15,
} VCPU_CONTEXT_REG;

typedef enum _VCPU_CONTEXT_SEG
{
  ContextSegEs                                                = 0,
  ContextSegCs                                                = 1,
  ContextSegSs                                                = 2,
  ContextSegDs                                                = 3,
  ContextSegFs                                                = 4,
  ContextSegGs                                                = 5,
  ContextSegLdtr                                              = 6,
  ContextSegTr                                                = 7,

  ContextSegMin                                               = 0,
  ContextSegMax                                               = 7,
} VCPU_CONTEXT_SEG;

typedef struct _VCPU_CONTEXT
{
  union
  {
    struct
    {
      union
      {
        ULONG64 Rax;
        PVOID   RaxAsPointer;

        struct
        {
          ULONG Eax;
          ULONG RaxHigh;
        };
      };

      union
      {
        ULONG64 Rcx;
        PVOID   RcxAsPointer;

        struct
        {
          ULONG Ecx;
          ULONG RcxHigh;
        };
      };

      union
      {
        ULONG64 Rdx;
        PVOID   RdxAsPointer;

        struct
        {
          ULONG Edx;
          ULONG RdxHigh;
        };
      };

      union
      {
        ULONG64 Rbx;
        PVOID   RbxAsPointer;

        struct
        {
          ULONG Ebx;
          ULONG RbxHigh;
        };
      };

      union
      {
        ULONG64 Rsp;
        PVOID   RspAsPointer;

        struct
        {
          ULONG Esp;
          ULONG RspHigh;
        };
      };

      union
      {
        ULONG64 Rbp;
        PVOID   RbpAsPointer;

        struct
        {
          ULONG Ebp;
          ULONG RbpHigh;
        };
      };

      union
      {
        ULONG64 Rsi;
        PVOID   RsiAsPointer;

        struct
        {
          ULONG Esi;
          ULONG RsiHigh;
        };
      };

      union
      {
        ULONG64 Rdi;
        PVOID   RdiAsPointer;

        struct
        {
          ULONG Edi;
          ULONG RdiHigh;
        };
      };

      union
      {
        ULONG64 R8;
        PVOID   R8AsPointer;

        struct
        {
          ULONG R8d;
          ULONG R8High;
        };
      };

      union
      {
        ULONG64 R9;
        PVOID   R9AsPointer;

        struct
        {
          ULONG R9d;
          ULONG R9High;
        };
      };

      union
      {
        ULONG64 R10;
        PVOID   R10AsPointer;

        struct
        {
          ULONG R10d;
          ULONG R10High;
        };
      };

      union
      {
        ULONG64 R11;
        PVOID   R11AsPointer;

        struct
        {
          ULONG R11d;
          ULONG R11High;
        };
      };

      union
      {
        ULONG64 R12;
        PVOID   R12AsPointer;

        struct
        {
          ULONG R12d;
          ULONG R12High;
        };
      };

      union
      {
        ULONG64 R13;
        PVOID   R13AsPointer;

        struct
        {
          ULONG R13d;
          ULONG R13High;
        };
      };

      union
      {
        ULONG64 R14;
        PVOID   R14AsPointer;

        struct
        {
          ULONG R14d;
          ULONG R14High;
        };
      };

      union
      {
        ULONG64 R15;
        PVOID   R15AsPointer;

        struct
        {
          ULONG R15d;
          ULONG R15High;
        };
      };
    };

    ULONG64 GpRegister[16];
  };

  union
  {
    ULONG64 Rip;
    PVOID   RipAsPointer;

    struct
    {
      ULONG Eip;
      ULONG RipHigh;
    };
  };

  ULONG64 Rflags;
} VCPU_CONTEXT, *PVCPU_CONTEXT;

#pragma endregion

#pragma region vmx/exception_bitmap.h

//
// Already defined in the WDK:
//   EXCEPTION_DIVIDED_BY_ZERO
//   EXCEPTION_DEBUG
//   EXCEPTION_NMI
//   EXCEPTION_INT3
//   EXCEPTION_BOUND_CHECK
//   EXCEPTION_INVALID_OPCODE
//   EXCEPTION_NPX_NOT_AVAILABLE
//   EXCEPTION_DOUBLE_FAULT
//   EXCEPTION_NPX_OVERRUN
//   EXCEPTION_INVALID_TSS
//   EXCEPTION_SEGMENT_NOT_PRESENT
//   EXCEPTION_STACK_FAULT
//   EXCEPTION_GP_FAULT
//   EXCEPTION_RESERVED_TRAP
//   EXCEPTION_NPX_ERROR
//   EXCEPTION_ALIGNMENT_CHECK
//   EXCEPTION_CP_FAULT
//   EXCEPTION_VIRTUALIZATION_FAULT
//
//
// typedef enum _EXCEPTION
// {
//   EXCEPTION_DIVIDE_ERROR                                      =  0,
//   EXCEPTION_DEBUG                                             =  1,
//   EXCEPTION_NMI_INTERRUPT                                     =  2,
//   EXCEPTION_BREAKPOINT                                        =  3,
//   EXCEPTION_OVERFLOW                                          =  4,
//   EXCEPTION_BOUND                                             =  5,
//   EXCEPTION_INVALID_OPCODE                                    =  6,
//   EXCEPTION_DEVICE_NOT_AVAILABLE                              =  7,
//   EXCEPTION_DOUBLE_FAULT                                      =  8,
//   EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN                       =  9,
//   EXCEPTION_INVALID_TSS                                       = 10,
//   EXCEPTION_SEGMENT_NOT_PRESENT                               = 11,
//   EXCEPTION_STACK_SEGMENT_FAULT                               = 12,
//   EXCEPTION_GENERAL_PROTECTION                                = 13,
//   EXCEPTION_PAGE_FAULT                                        = 14,
//   EXCEPTION_RESERVED                                          = 15,
//   EXCEPTION_X87_FLOATING_POINT_ERROR                          = 16,
//   EXCEPTION_ALIGNMENT_CHECK                                   = 17,
//   EXCEPTION_MACHINE_CHECK                                     = 18,
//   EXCEPTION_SIMD_FLOATING_POINT_ERROR                         = 19,
//   EXCEPTION_VIRTUALIZATION_EXCEPTION                          = 20,
// } EXCEPTION;
//

#pragma endregion

#pragma region vmx/exit_qualification.h

typedef struct _VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 BreakpointCondition : 4;
      ULONG64 Reserved1 : 9;
      ULONG64 DebugRegisterAccessDetected : 1;
      ULONG64 SingleInstruction : 1;
    };
  };
} VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION, *PVMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION;

typedef struct _VMX_EXIT_QUALIFICATION_TASK_SWITCH
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 Selector : 16;
      ULONG64 Reserved1 : 14;
      ULONG64 Type : 2;
    };
  };
} VMX_EXIT_QUALIFICATION_TASK_SWITCH, *PVMX_EXIT_QUALIFICATION_TASK_SWITCH;


#define VMX_EXIT_ACCESS_TO_CR                                 0
#define VMX_EXIT_ACCESS_FROM_CR                               1
#define VMX_EXIT_ACCESS_CLTS                                  2
#define VMX_EXIT_ACCESS_LMSW                                  3

typedef struct _VMX_EXIT_QUALIFICATION_MOV_CR
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 CrNumber : 4;
      ULONG64 AccessType : 2;
      ULONG64 LmswOperandType : 1;
      ULONG64 Reserved1 : 1;
      ULONG64 GpRegister : 4;
      ULONG64 Reserved2 : 4;
      ULONG64 LmswSourceData : 16;
    };
  };
} VMX_EXIT_QUALIFICATION_MOV_CR, *PVMX_EXIT_QUALIFICATION_MOV_CR;


#define VMX_ACCESS_TO_DR                                      0
#define VMX_ACCESS_FROM_DR                                    1

typedef struct _VMX_EXIT_QUALIFICATION_MOV_DR
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 DrNumber : 3;
      ULONG64 Reserved1 : 1;
      ULONG64 AccessType : 1;
      ULONG64 Reserved2 : 3;
      ULONG64 GpRegister : 4;
    };
  };
} VMX_EXIT_QUALIFICATION_MOV_DR, *PVMX_EXIT_QUALIFICATION_MOV_DR;

#define VMX_EXIT_ACCESS_OUT                                   0
#define VMX_EXIT_ACCESS_IN                                    1

#define VMX_OP_ENCODING_DX                                    0
#define VMX_OP_ENCODING_IMM                                   1

typedef struct _VMX_EXIT_QUALIFICATION_IO_INSTRUCTION
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 SizeOfAccess : 3;
      ULONG64 AccessType : 1;
      ULONG64 StringInstruction : 1;
      ULONG64 RepPrefixed : 1;
      ULONG64 OperandEncoding : 1;
      ULONG64 Reserved1 : 9;
      ULONG64 PortNumber : 16;
    };
  };
} VMX_EXIT_QUALIFICATION_IO_INSTRUCTION, *PVMX_EXIT_QUALIFICATION_IO_INSTRUCTION;

typedef struct _VMX_EXIT_QUALIFICATION_APIC_ACCESS
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 PageOffset : 12;
      ULONG64 AccessType : 4;
    };
  };
} VMX_EXIT_QUALIFICATION_APIC_ACCESS, *PVMX_EXIT_QUALIFICATION_APIC_ACCESS;

typedef struct _VMX_EXIT_QUALIFICATION_EPT_VIOLATION
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 DataRead : 1;
      ULONG64 DataWrite : 1;
      ULONG64 DataExecute : 1;
      ULONG64 EntryRead : 1;
      ULONG64 EntryWrite : 1;
      ULONG64 EntryExecute : 1;
      ULONG64 EntryExecuteForUserMode : 1;
      ULONG64 ValidGuestLinearAddress : 1;
      ULONG64 EptTranslatedAccess : 1;
      ULONG64 UserModeLinearAddress : 1;
      ULONG64 ReadableWritablePage : 1;
      ULONG64 ExecuteDisablePage : 1;
      ULONG64 NmiUnblocking : 1;
    };
  };
} VMX_EXIT_QUALIFICATION_EPT_VIOLATION, *PVMX_EXIT_QUALIFICATION_EPT_VIOLATION;

typedef struct _VMX_EXIT_QUALIFICATION
{
  union
  {
    ULONG64 Flags;

    ULONG64                                 Displacement;
    ULONG64                                 LinearAddress;

    VMX_EXIT_QUALIFICATION_DEBUG_EXCEPTION  DebugException;
    VMX_EXIT_QUALIFICATION_TASK_SWITCH      TaskSwitch;
    VMX_EXIT_QUALIFICATION_MOV_CR           MovCr;
    VMX_EXIT_QUALIFICATION_MOV_DR           MovDr;
    VMX_EXIT_QUALIFICATION_IO_INSTRUCTION   IoInstruction;
    VMX_EXIT_QUALIFICATION_APIC_ACCESS      ApicAccess;
    VMX_EXIT_QUALIFICATION_EPT_VIOLATION    EptViolation;
  };
} VMX_EXIT_QUALIFICATION, *PVMX_EXIT_QUALIFICATION;


#pragma endregion

#pragma region vmx/exit_reason.h

typedef enum _VMEXIT_REASON
{
  REASON_ERROR_MACHINE_CHECK                                  = 0x0029,
  VMEXIT_REASON_EXCEPTION_OR_NMI                              = 0x0000,
  VMEXIT_REASON_EXTERNAL_INTERRUPT                            = 0x0001,
  VMEXIT_REASON_TRIPLE_FAULT                                  = 0x0002,
  VMEXIT_REASON_INIT_SIGNAL                                   = 0x0003,
  VMEXIT_REASON_STARTUP_IPI                                   = 0x0004,
  VMEXIT_REASON_IO_SMI                                        = 0x0005,
  VMEXIT_REASON_SMI                                           = 0x0006,
  VMEXIT_REASON_INTERRUPT_WINDOW                              = 0x0007,
  VMEXIT_REASON_NMI_WINDOW                                    = 0x0008,
  VMEXIT_REASON_TASK_SWITCH                                   = 0x0009,
  VMEXIT_REASON_EXECUTE_CPUID                                 = 0x000a,
  VMEXIT_REASON_EXECUTE_GETSEC                                = 0x000b,
  VMEXIT_REASON_EXECUTE_HLT                                   = 0x000c,
  VMEXIT_REASON_EXECUTE_INVD                                  = 0x000d,
  VMEXIT_REASON_EXECUTE_INVLPG                                = 0x000e,
  VMEXIT_REASON_EXECUTE_RDPMC                                 = 0x000f,
  VMEXIT_REASON_EXECUTE_RDTSC                                 = 0x0010,
  VMEXIT_REASON_EXECUTE_RSM_IN_SMM                            = 0x0011,
  VMEXIT_REASON_EXECUTE_VMCALL                                = 0x0012,
  VMEXIT_REASON_EXECUTE_VMCLEAR                               = 0x0013,
  VMEXIT_REASON_EXECUTE_VMLAUNCH                              = 0x0014,
  VMEXIT_REASON_EXECUTE_VMPTRLD                               = 0x0015,
  VMEXIT_REASON_EXECUTE_VMPTRST                               = 0x0016,
  VMEXIT_REASON_EXECUTE_VMREAD                                = 0x0017,
  VMEXIT_REASON_EXECUTE_VMRESUME                              = 0x0018,
  VMEXIT_REASON_EXECUTE_VMWRITE                               = 0x0019,
  VMEXIT_REASON_EXECUTE_VMXOFF                                = 0x001a,
  VMEXIT_REASON_EXECUTE_VMXON                                 = 0x001b,
  VMEXIT_REASON_MOV_CR                                        = 0x001c,
  VMEXIT_REASON_MOV_DR                                        = 0x001d,
  VMEXIT_REASON_EXECUTE_IO_INSTRUCTION                        = 0x001e,
  VMEXIT_REASON_EXECUTE_RDMSR                                 = 0x001f,
  VMEXIT_REASON_EXECUTE_WRMSR                                 = 0x0020,
  VMEXIT_REASON_ERROR_INVALID_GUEST_STATE                     = 0x0021,
  VMEXIT_REASON_ERROR_MSR_LOAD                                = 0x0022,
  VMEXIT_REASON_RESERVED_1                                    = 0x0023,
  VMEXIT_REASON_EXECUTE_MWAIT                                 = 0x0024,
  VMEXIT_REASON_MONITOR_TRAP_FLAG                             = 0x0025,
  VMEXIT_REASON_RESERVED_2                                    = 0x0026,
  VMEXIT_REASON_EXECUTE_MONITOR                               = 0x0027,
  VMEXIT_REASON_EXECUTE_PAUSE                                 = 0x0028,
  VMEXIT_REASON_RESERVED_3                                    = 0x002a,
  VMEXIT_REASON_TPR_BELOW_THRESHOLD                           = 0x002b,
  VMEXIT_REASON_APIC_ACCESS                                   = 0x002c,
  VMEXIT_REASON_VIRTUALIZED_EOI                               = 0x002d,
  VMEXIT_REASON_GDTR_IDTR_ACCESS                              = 0x002e,
  VMEXIT_REASON_LDTR_TR_ACCESS                                = 0x002f,
  VMEXIT_REASON_EPT_VIOLATION                                 = 0x0030,
  VMEXIT_REASON_EPT_MISCONFIGURATION                          = 0x0031,
  VMEXIT_REASON_EXECUTE_INVEPT                                = 0x0032,
  VMEXIT_REASON_EXECUTE_RDTSCP                                = 0x0033,
  VMEXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED                  = 0x0034,
  VMEXIT_REASON_EXECUTE_INVVPID                               = 0x0035,
  VMEXIT_REASON_EXECUTE_WBINVD                                = 0x0036,
  VMEXIT_REASON_EXECUTE_XSETBV                                = 0x0037,
  VMEXIT_REASON_APIC_WRITE                                    = 0x0038,
  VMEXIT_REASON_EXECUTE_RDRAND                                = 0x0039,
  VMEXIT_REASON_EXECUTE_INVPCID                               = 0x003a,
  VMEXIT_REASON_EXECUTE_VMFUNC                                = 0x003b,
  VMEXIT_REASON_EXECUTE_ENCLS                                 = 0x003c,
  VMEXIT_REASON_EXECUTE_RDSEED                                = 0x003d,
  VMEXIT_REASON_PAGE_MODIFICATION_LOG_FULL                    = 0x003e,
  VMEXIT_REASON_EXECUTE_XSAVES                                = 0x003f,
  VMEXIT_REASON_EXECUTE_XRSTORS                               = 0x0040,

  VMEXIT_REASON_MAX
} VMEXIT_REASON;

#pragma endregion

#pragma region vmx/vmcs.h

typedef enum _VMCS_FIELD
{
  VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER                      = 0x0000,
  VMCS_CTRL_POSTED_INTERRUPT_NOTIFICATION_VECTOR              = 0x0002,
  VMCS_CTRL_EPTP_INDEX                                        = 0x0004,

  //
  // control::64_bit
  //

  VMCS_CTRL_IO_BITMAP_A_ADDRESS                               = 0x2000,
  VMCS_CTRL_IO_BITMAP_B_ADDRESS                               = 0x2002,
  VMCS_CTRL_MSR_BITMAP_ADDRESS                                = 0x2004,
  VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS                          = 0x2006,
  VMCS_CTRL_VMEXIT_MSR_LOAD_ADDRESS                           = 0x2008,
  VMCS_CTRL_VMENTRY_MSR_LOAD_ADDRESS                          = 0x200A,
  VMCS_CTRL_EXECUTIVE_VMCS_POINTER                            = 0x200C,
  VMCS_CTRL_PML_ADDRESS                                       = 0x200E,
  VMCS_CTRL_TSC_OFFSET                                        = 0x2010,
  VMCS_CTRL_VIRTUAL_APIC_ADDRESS                              = 0x2012,
  VMCS_CTRL_APIC_ACCESS_ADDRESS                               = 0x2014,
  VMCS_CTRL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS               = 0x2016,
  VMCS_CTRL_VMFUNC_CONTROLS                                   = 0x2018,
  VMCS_CTRL_EPT_POINTER                                       = 0x201A,
  VMCS_CTRL_EOI_EXIT_BITMAP_0                                 = 0x201C,
  VMCS_CTRL_EOI_EXIT_BITMAP_1                                 = 0x201E,
  VMCS_CTRL_EOI_EXIT_BITMAP_2                                 = 0x2020,
  VMCS_CTRL_EOI_EXIT_BITMAP_3                                 = 0x2022,
  VMCS_CTRL_EPT_POINTER_LIST_ADDRESS                          = 0x2024,
  VMCS_CTRL_VMREAD_BITMAP_ADDRESS                             = 0x2026,
  VMCS_CTRL_VMWRITE_BITMAP_ADDRESS                            = 0x2028,
  VMCS_CTRL_VIRTUALIZATION_EXCEPTION_INFO_ADDRESS             = 0x202A,
  VMCS_CTRL_XSS_EXITING_BITMAP                                = 0x202C,
  VMCS_CTRL_ENCLS_EXITING_BITMAP                              = 0x202E,
  VMCS_CTRL_TSC_MULTIPLIER                                    = 0x2032,

  //
  // control::32_bit
  //

  VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS                   = 0x4000,
  VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS             = 0x4002,
  VMCS_CTRL_EXCEPTION_BITMAP                                  = 0x4004,
  VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK                         = 0x4006,
  VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH                        = 0x4008,
  VMCS_CTRL_CR3_TARGET_COUNT                                  = 0x400A,
  VMCS_CTRL_VMEXIT_CONTROLS                                   = 0x400C,
  VMCS_CTRL_VMEXIT_MSR_STORE_COUNT                            = 0x400E,
  VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT                             = 0x4010,
  VMCS_CTRL_VMENTRY_CONTROLS                                  = 0x4012,
  VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT                            = 0x4014,
  VMCS_CTRL_VMENTRY_INTERRUPTION_INFO                         = 0x4016,
  VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE                      = 0x4018,
  VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH                        = 0x401A,
  VMCS_CTRL_TPR_THRESHOLD                                     = 0x401C,
  VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS   = 0x401E,
  VMCS_CTRL_PLE_GAP                                           = 0x4020,
  VMCS_CTRL_PLE_WINDOW                                        = 0x4022,

  //
  // control::natural
  //

  VMCS_CTRL_CR0_GUEST_HOST_MASK                               = 0x6000,
  VMCS_CTRL_CR4_GUEST_HOST_MASK                               = 0x6002,
  VMCS_CTRL_CR0_READ_SHADOW                                   = 0x6004,
  VMCS_CTRL_CR4_READ_SHADOW                                   = 0x6006,
  VMCS_CTRL_CR3_TARGET_VALUE_0                                = 0x6008,
  VMCS_CTRL_CR3_TARGET_VALUE_1                                = 0x600A,
  VMCS_CTRL_CR3_TARGET_VALUE_2                                = 0x600C,
  VMCS_CTRL_CR3_TARGET_VALUE_3                                = 0x600E,

  //
  // [VMEXIT] (read-only)
  //

  //
  // vmexit::16_bit
  //

  //
  // vmexit::64_bit
  //

  VMCS_VMEXIT_GUEST_PHYSICAL_ADDRESS                          = 0x2400,

  //
  // vmexit::32_bit
  //

  VMCS_VMEXIT_INSTRUCTION_ERROR                               = 0x4400,
  VMCS_VMEXIT_REASON                                          = 0x4402,
  VMCS_VMEXIT_INTERRUPTION_INFO                               = 0x4404,
  VMCS_VMEXIT_INTERRUPTION_ERROR_CODE                         = 0x4406,
  VMCS_VMEXIT_IDT_VECTORING_INFO                              = 0x4408,
  VMCS_VMEXIT_IDT_VECTORING_ERROR_CODE                        = 0x440A,
  VMCS_VMEXIT_INSTRUCTION_LENGTH                              = 0x440C,
  VMCS_VMEXIT_INSTRUCTION_INFO                                = 0x440E,

  //
  // vmexit::natural
  //

  VMCS_VMEXIT_QUALIFICATION                                   = 0x6400,
  VMCS_VMEXIT_IO_RCX                                          = 0x6402,
  VMCS_VMEXIT_IO_RSX                                          = 0x6404,
  VMCS_VMEXIT_IO_RDI                                          = 0x6406,
  VMCS_VMEXIT_IO_RIP                                          = 0x6408,
  VMCS_VMEXIT_GUEST_LINEAR_ADDRESS                            = 0x640A,

  //
  // [GUEST]
  //

  //
  // guest::16_bit
  //

  VMCS_GUEST_ES_SELECTOR                                      = 0x0800,
  VMCS_GUEST_CS_SELECTOR                                      = 0x0802,
  VMCS_GUEST_SS_SELECTOR                                      = 0x0804,
  VMCS_GUEST_DS_SELECTOR                                      = 0x0806,
  VMCS_GUEST_FS_SELECTOR                                      = 0x0808,
  VMCS_GUEST_GS_SELECTOR                                      = 0x080A,
  VMCS_GUEST_LDTR_SELECTOR                                    = 0x080C,
  VMCS_GUEST_TR_SELECTOR                                      = 0x080E,
  VMCS_GUEST_INTERRUPT_STATUS                                 = 0x0810,
  VMCS_GUEST_PML_INDEX                                        = 0x0812,

  //
  // guest::64_bit
  //

  VMCS_GUEST_VMCS_LINK_POINTER                                = 0x2800,
  VMCS_GUEST_DEBUGCTL                                         = 0x2802,
  VMCS_GUEST_PAT                                              = 0x2804,
  VMCS_GUEST_EFER                                             = 0x2806,
  VMCS_GUEST_PERF_GLOBAL_CTRL                                 = 0x2808,
  VMCS_GUEST_PDPTE0                                           = 0x280A,
  VMCS_GUEST_PDPTE1                                           = 0x280C,
  VMCS_GUEST_PDPTE2                                           = 0x280E,
  VMCS_GUEST_PDPTE3                                           = 0x2810,

  //
  // guest::32_bit
  //

  VMCS_GUEST_ES_LIMIT                                         = 0x4800,
  VMCS_GUEST_CS_LIMIT                                         = 0x4802,
  VMCS_GUEST_SS_LIMIT                                         = 0x4804,
  VMCS_GUEST_DS_LIMIT                                         = 0x4806,
  VMCS_GUEST_FS_LIMIT                                         = 0x4808,
  VMCS_GUEST_GS_LIMIT                                         = 0x480A,
  VMCS_GUEST_LDTR_LIMIT                                       = 0x480C,
  VMCS_GUEST_TR_LIMIT                                         = 0x480E,
  VMCS_GUEST_GDTR_LIMIT                                       = 0x4810,
  VMCS_GUEST_IDTR_LIMIT                                       = 0x4812,
  VMCS_GUEST_ES_ACCESS_RIGHTS                                 = 0x4814,
  VMCS_GUEST_CS_ACCESS_RIGHTS                                 = 0x4816,
  VMCS_GUEST_SS_ACCESS_RIGHTS                                 = 0x4818,
  VMCS_GUEST_DS_ACCESS_RIGHTS                                 = 0x481A,
  VMCS_GUEST_FS_ACCESS_RIGHTS                                 = 0x481C,
  VMCS_GUEST_GS_ACCESS_RIGHTS                                 = 0x481E,
  VMCS_GUEST_LDTR_ACCESS_RIGHTS                               = 0x4820,
  VMCS_GUEST_TR_ACCESS_RIGHTS                                 = 0x4822,
  VMCS_GUEST_INTERRUPTIBILITY_STATE                           = 0x4824,
  VMCS_GUEST_ACTIVITY_STATE                                   = 0x4826,
  VMCS_GUEST_SMBASE                                           = 0x4828,
  VMCS_GUEST_SYSENTER_CS                                      = 0x482A,
  VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE                       = 0x482E,

  //
  // guest::natural
  //

  VMCS_GUEST_CR0                                              = 0x6800,
  VMCS_GUEST_CR3                                              = 0x6802,
  VMCS_GUEST_CR4                                              = 0x6804,
  VMCS_GUEST_ES_BASE                                          = 0x6806,
  VMCS_GUEST_CS_BASE                                          = 0x6808,
  VMCS_GUEST_SS_BASE                                          = 0x680A,
  VMCS_GUEST_DS_BASE                                          = 0x680C,
  VMCS_GUEST_FS_BASE                                          = 0x680E,
  VMCS_GUEST_GS_BASE                                          = 0x6810,
  VMCS_GUEST_LDTR_BASE                                        = 0x6812,
  VMCS_GUEST_TR_BASE                                          = 0x6814,
  VMCS_GUEST_GDTR_BASE                                        = 0x6816,
  VMCS_GUEST_IDTR_BASE                                        = 0x6818,
  VMCS_GUEST_DR7                                              = 0x681A,
  VMCS_GUEST_RSP                                              = 0x681C,
  VMCS_GUEST_RIP                                              = 0x681E,
  VMCS_GUEST_RFLAGS                                           = 0x6820,
  VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS                         = 0x6822,
  VMCS_GUEST_SYSENTER_ESP                                     = 0x6824,
  VMCS_GUEST_SYSENTER_EIP                                     = 0x6826,

  //
  // [HOST]
  //

  //
  // host::16_bit
  //

  VMCS_HOST_ES_SELECTOR                                       = 0x0C00,
  VMCS_HOST_CS_SELECTOR                                       = 0x0C02,
  VMCS_HOST_SS_SELECTOR                                       = 0x0C04,
  VMCS_HOST_DS_SELECTOR                                       = 0x0C06,
  VMCS_HOST_FS_SELECTOR                                       = 0x0C08,
  VMCS_HOST_GS_SELECTOR                                       = 0x0C0A,
  VMCS_HOST_TR_SELECTOR                                       = 0x0C0C,

  //
  // host::64_bit
  //

  VMCS_HOST_PAT                                               = 0x2C00,
  VMCS_HOST_EFER                                              = 0x2C02,
  VMCS_HOST_PERF_GLOBAL_CTRL                                  = 0x2C04,

  //
  // host::32_bit
  //

  VMCS_HOST_SYSENTER_CS                                       = 0x4C00,

  //
  // host::natural
  //

  VMCS_HOST_CR0                                               = 0x6C00,
  VMCS_HOST_CR3                                               = 0x6C02,
  VMCS_HOST_CR4                                               = 0x6C04,
  VMCS_HOST_FS_BASE                                           = 0x6C06,
  VMCS_HOST_GS_BASE                                           = 0x6C08,
  VMCS_HOST_TR_BASE                                           = 0x6C0A,
  VMCS_HOST_GDTR_BASE                                         = 0x6C0C,
  VMCS_HOST_IDTR_BASE                                         = 0x6C0E,
  VMCS_HOST_SYSENTER_ESP                                      = 0x6C10,
  VMCS_HOST_SYSENTER_EIP                                      = 0x6C12,
  VMCS_HOST_RSP                                               = 0x6C14,
  VMCS_HOST_RIP                                               = 0x6C16,
} VMCS_FIELD;

#pragma endregion

#pragma endregion

#pragma region Structures and function prototypes

//////////////////////////////////////////////////////////////////////////
// Opaque type definitions.
//////////////////////////////////////////////////////////////////////////

typedef PVOID PVCPU;
typedef PVOID PEPT;

//////////////////////////////////////////////////////////////////////////
// VM-exit pass-through handler.
//////////////////////////////////////////////////////////////////////////

typedef struct _VMEXIT_PASSTHROUGH
{
  PVOID PassthroughRoutine;
  PVOID Context;
  // UCHAR Data[1];
} VMEXIT_PASSTHROUGH, *PVMEXIT_PASSTHROUGH;

#define HvppPassthroughContext(Passthrough)                   \
  ((PVMEXIT_PASSTHROUGH)(Passthrough)->Context)

//
// Setup.
//

typedef NTSTATUS (NTAPI* PVMEXIT_PASSTHROUGH_SETUP_ROUTINE)(
  _In_ PVOID PassthroughContext
  );

#define HvppPassthroughSetup(Passthrough)                     \
  (((PVMEXIT_PASSTHROUGH_SETUP_ROUTINE)(((PVMEXIT_PASSTHROUGH)(Passthrough))->PassthroughRoutine))(Passthrough));

//
// Teardown.
//

typedef VOID (NTAPI* PVMEXIT_PASSTHROUGH_TEARDOWN_ROUTINE)(
  _In_ PVOID PassthroughContext
  );

#define HvppPassthroughTeardown(Passthrough)                  \
  (((PVMEXIT_PASSTHROUGH_TEARDOWN_ROUTINE)(((PVMEXIT_PASSTHROUGH)(Passthrough))->PassthroughRoutine))(Passthrough));

//
// Terminate.
//

typedef VOID (NTAPI* PVMEXIT_PASSTHROUGH_TERMINATE_ROUTINE)(
  _In_ PVOID PassthroughContext
  );

#define HvppPassthroughTerminate(Passthrough)                 \
  (((PVMEXIT_PASSTHROUGH_TERMINATE_ROUTINE)(((PVMEXIT_PASSTHROUGH)(Passthrough))->PassthroughRoutine))(Passthrough));

//
// Handler.
//

typedef VOID (NTAPI* PVMEXIT_PASSTHROUGH_HANDLER_ROUTINE)(
  _In_ PVOID PassthroughContext
  );

#define HvppPassthroughHandler(Passthrough)                   \
  (((PVMEXIT_PASSTHROUGH_HANDLER_ROUTINE)(((PVMEXIT_PASSTHROUGH)(Passthrough))->PassthroughRoutine))(Passthrough));

//////////////////////////////////////////////////////////////////////////
// VM-exit handler.
//////////////////////////////////////////////////////////////////////////

typedef VOID (NTAPI* PVMEXIT_HANDLER_ROUTINE)(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

typedef NTSTATUS (NTAPI* PVMEXIT_HANDLER_SETUP_ROUTINE)(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

typedef VOID (NTAPI* PVMEXIT_HANDLER_TEARDOWN_ROUTINE)(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

typedef VOID (NTAPI* PVMEXIT_HANDLER_TERMINATE_ROUTINE)(
  _In_ PVCPU Vcpu,
  _In_ PVOID Passthrough
  );

typedef struct _VMEXIT_HANDLER
{
  PVMEXIT_HANDLER_ROUTINE HandlerRoutine[VMEXIT_REASON_MAX];
  PVOID Context;
} VMEXIT_HANDLER, *PVMEXIT_HANDLER;

//////////////////////////////////////////////////////////////////////////
// Structures.
//////////////////////////////////////////////////////////////////////////

typedef struct __declspec(align(PAGE_SIZE)) _MSR_BITMAP
{
  union
  {
    struct
    {
      UCHAR RdmsrLow [PAGE_SIZE / 4];
      UCHAR RdmsrHigh[PAGE_SIZE / 4];
      UCHAR WrmsrLow [PAGE_SIZE / 4];
      UCHAR WrmsrHigh[PAGE_SIZE / 4];
    };

    UCHAR Data[PAGE_SIZE];
  };
} MSR_BITMAP, *PMSR_BITMAP;

#define IO_BITMAP_A_MIN                                       0x0000
#define IO_BITMAP_A_MAX                                       0x7FFF
#define IO_BITMAP_B_MIN                                       0x8000
#define IO_BITMAP_B_MAX                                       0xFFFF

typedef struct __declspec(align(PAGE_SIZE)) _IO_BITMAP
{
  union
  {
    struct
    {
      UCHAR A[PAGE_SIZE];
      UCHAR B[PAGE_SIZE];
    };

    UCHAR Data[2 * PAGE_SIZE];
  };
} IO_BITMAP, *PIO_BITMAP;

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// ept.h
//////////////////////////////////////////////////////////////////////////

#pragma region ept.h

typedef struct _EPT_PTE
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 MemoryType : 3;
      ULONG64 IgnorePat : 1;
      ULONG64 Reserved1 : 1;
      ULONG64 Accessed : 1;
      ULONG64 Dirty : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved2 : 1;
      ULONG64 PageFrameNumber : 36;
      ULONG64 Reserved3 : 15;
      ULONG64 SuppressVe : 1;
    };
  };
} EPT_PTE, *PEPT_PTE;

typedef struct _EPT_PDE
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 Reserved1 : 5;
      ULONG64 Accessed : 1;
      ULONG64 Reserved2 : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved3 : 1;
      ULONG64 PageFrameNumber : 36;
    };
  };
} EPT_PDE, *PEPT_PDE;

typedef struct _EPT_PDE_LARGE
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 MemoryType : 3;
      ULONG64 IgnorePat : 1;
      ULONG64 LargePage : 1;
      ULONG64 Accessed : 1;
      ULONG64 Dirty : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved1 : 10;
      ULONG64 PageFrameNumber : 27;
      ULONG64 Reserved2 : 15;
      ULONG64 SuppressVe : 1;
    };
  };
} EPT_PDE_LARGE, *PEPT_PDE_LARGE;

typedef struct _EPT_PDPTE
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 Reserved1 : 5;
      ULONG64 Accessed : 1;
      ULONG64 Reserved2 : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved3 : 1;
      ULONG64 PageFrameNumber : 36;
    };
  };
} EPT_PDPTE, *PEPT_PDPTE;

typedef struct _EPT_PDPTE_LARGE
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 MemoryType : 3;
      ULONG64 IgnorePat : 1;
      ULONG64 LargePage : 1;
      ULONG64 Accessed : 1;
      ULONG64 Dirty : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved1 : 19;
      ULONG64 PageFrameNumber : 18;
      ULONG64 Reserved2 : 15;
      ULONG64 SuppressVe : 1;
    };
  };
} EPT_PDPTE_LARGE, *PEPT_PDPTE_LARGE;

typedef struct _EPT_PML4E
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 Reserved1 : 5;
      ULONG64 Accessed : 1;
      ULONG64 Reserved2 : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved3 : 1;
      ULONG64 PageFrameNumber : 36;
    };
  };
} EPT_PML4E, *PEPT_PML4E;

#define EPT_PAGE_WALK_LENGTH_4                                3

typedef struct _EPT_PTR
{
  union
  {
    ULONG64 Flags;

    struct
    {
      ULONG64 MemoryType : 3;
      ULONG64 PageWalkLength : 3;
      ULONG64 EnableAccessAndDirtyFlags : 1;
      ULONG64 Reserved1 : 5;
      ULONG64 PageFrameNumber : 36;
    };
  };
} EPT_PTR, *PEPT_PTR;

#define EPT_ACCESS_NONE                                       0
#define EPT_ACCESS_READ                                       1
#define EPT_ACCESS_WRITE                                      2
#define EPT_ACCESS_EXECUTE                                    4

#define EPT_ACCESS_READ_WRITE                                 (EPT_ACCESS_READ  | \
                                                               EPT_ACCESS_WRITE)

#define EPT_ACCESS_READ_EXECUTE                               (EPT_ACCESS_READ  | \
                                                               EPT_ACCESS_EXECUTE)

#define EPT_ACCESS_READ_WRITE_EXECUTE                         (EPT_ACCESS_READ  | \
                                                               EPT_ACCESS_WRITE | \
                                                               EPT_ACCESS_EXECUTE)

#define EPT_ACCESS_WRITE_EXECUTE                              (EPT_ACCESS_WRITE | \
                                                               EPT_ACCESS_EXECUTE)

#define EPT_ACCESS_MASK                                       7,

typedef struct _EPTE
{
  union
  {
    ULONG64 Flags;

    EPT_PML4E Pml4;
    EPT_PDPTE_LARGE PdptLarge;
    EPT_PDPTE Pdpt;
    EPT_PDE_LARGE PdLarge;
    EPT_PDE Pd;
    EPT_PTE Pt;

    struct
    {
      ULONG64 ReadAccess : 1;
      ULONG64 WriteAccess : 1;
      ULONG64 ExecuteAccess : 1;
      ULONG64 MemoryType : 3;
      ULONG64 IgnorePat : 1;
      ULONG64 LargePage : 1;
      ULONG64 Accessed : 1;
      ULONG64 Dirty : 1;
      ULONG64 UserModeExecute : 1;
      ULONG64 Reserved1 : 1;
      ULONG64 PageFrameNumber : 36;
      ULONG64 Reserved2 : 15;
      ULONG64 SuppressVe : 1;
    };

    struct
    {
      ULONG64 Access : 3;
    };
  };
} EPTE, *PEPTE;

PEPT
NTAPI
HvppEptCreate(
  VOID
  );

VOID
NTAPI
HvppEptDestroy(
  _In_ PEPT Ept
  );

VOID
NTAPI
HvppEptMapIdentity(
  _In_ PEPT Ept
  );

VOID
NTAPI
HvppEptMapIdentityEx(
  _In_ PEPT Ept,
  _In_ ULONG Access
  );

VOID
NTAPI
HvppEptMapIdentitySparse(
  _In_ PEPT Ept
  );

VOID
NTAPI
HvppEptMapIdentitySparseEx(
  _In_ PEPT Ept,
  _In_ ULONG Access
  );

PEPTE
NTAPI
HvppEptMap(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  );

PEPTE
NTAPI
HvppEptMapEx(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access,
  _In_ ULONG Level
  );

PEPTE
NTAPI
HvppEptMap4Kb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  );

PEPTE
NTAPI
HvppEptMap2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  );

PEPTE
NTAPI
HvppEptMap1Gb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress,
  _In_ ULONG Access
  );

VOID
NTAPI
HvppEptSplit1GbTo2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  );

VOID
NTAPI
HvppEptSplit2MbTo4Kb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  );

VOID
NTAPI
HvppEptJoin2MbTo1Gb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  );

VOID
NTAPI
HvppEptJoin4KbTo2Mb(
  _In_ PEPT Ept,
  _In_ PHYSICAL_ADDRESS GuestPhysicalAddress,
  _In_ PHYSICAL_ADDRESS HostPhysicalAddress
  );

EPT_PTR
NTAPI
HvppEptGetEptPointer(
  _In_ PEPT Ept
  );

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// hypervisor.h
//////////////////////////////////////////////////////////////////////////

#pragma region hypervisor.h

NTSTATUS
NTAPI
HvppInitialize(
  VOID
  );

VOID
NTAPI
HvppDestroy(
  VOID
  );

NTSTATUS
NTAPI
HvppStart(
  _In_ PVMEXIT_HANDLER VmExitHandler,
  _In_ PVMEXIT_HANDLER_SETUP_ROUTINE SetupRoutine,
  _In_ PVMEXIT_HANDLER_TEARDOWN_ROUTINE TeardownRoutine,
  _In_ PVMEXIT_HANDLER_TERMINATE_ROUTINE TerminateRoutine
  );

VOID
NTAPI
HvppStop(
  VOID
  );

BOOLEAN
NTAPI
HvppIsRunning(
  VOID
  );

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// vcpu.h
//////////////////////////////////////////////////////////////////////////

#pragma region vcpu.h

VOID
NTAPI
HvppVcpuEnableEpt(
  _In_ PVCPU Vcpu
  );

VOID
NTAPI
HvppVcpuDisableEpt(
  _In_ PVCPU Vcpu
  );

PEPT
NTAPI
HvppVcpuGetEpt(
  _In_ PVCPU Vcpu
  );

VOID
NTAPI
HvppVcpuSetEpt(
  _In_ PVCPU Vcpu,
  _In_ PEPT Ept
  );

PVCPU_CONTEXT
NTAPI
HvppVcpuContext(
  _In_ PVCPU Vcpu
  );

VOID
NTAPI
HvppVcpuSuppressRipAdjust(
  _In_ PVCPU Vcpu
  );

PVOID
NTAPI
HvppVcpuGetUserData(
  _In_ PVCPU Vcpu
  );

VOID
NTAPI
HvppVcpuSetUserData(
  _In_ PVCPU Vcpu,
  _In_ PVOID UserData
  );

#pragma endregion

//////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////

#pragma region Helpers

PVOID
NTAPI
HvppAllocate(
  ULONG Size
  );

VOID
NTAPI
HvppFree(
  PVOID Address
  );

ULONG64
NTAPI
HvppVmRead(
  _In_ VMCS_FIELD VmcsField
  );

VOID
NTAPI
HvppVmWrite(
  _In_ VMCS_FIELD VmcsField,
  _In_ ULONG64 VmcsValue
  );

ULONG_PTR
NTAPI
HvppVmCall(
  _In_ ULONG_PTR Rcx,
  _In_ ULONG_PTR Rdx,
  _In_ ULONG_PTR R8,
  _In_ ULONG_PTR R9
  );

ULONG_PTR
NTAPI
HvppVmCallEx(
  _In_ ULONG_PTR Rcx,
  _In_ ULONG_PTR Rdx,
  _In_ ULONG_PTR R8,
  _In_ ULONG_PTR R9,
  _In_ ULONG_PTR R10,
  _In_ ULONG_PTR R11,
  _In_ ULONG_PTR R12,
  _In_ ULONG_PTR R13,
  _In_ ULONG_PTR R14,
  _In_ ULONG_PTR R15
  );

VOID
NTAPI
HvppInveptAll(
  VOID
  );

VOID
NTAPI
HvppInveptSingleContext(
  _In_ EPT_PTR EptPointer
  );

VOID
NTAPI
HvppAttachAddressSpace(
  _Inout_ ULONG_PTR* Cr3
  );

VOID
NTAPI
HvppDetachAddressSpace(
  _In_ ULONG_PTR Cr3
  );

VOID
HvppTrace(
  _In_ const CHAR* Format,
  ...
  );

#pragma endregion

#ifdef __cplusplus
}
#endif

#pragma warning(pop)
