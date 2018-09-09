#include "ia32/linux/common.S"

/*
 * Useful definitions.
 */

#define VCPU_OFFSET                 -0x8000     /* -vcpu_stack_size */
#define VCPU_LAUNCH_CONTEXT_OFFSET   0
#define VCPU_EXIT_CONTEXT_OFFSET     144        /* sizeof context */
#define SHADOW_SPACE                 0x20

/*
 * Externally used symbols.
 */
    /* "public:  __int64 __cdecl ia32::context_t::capture(void)" */
    .extern _ZN4ia329context_t7captureEv

    /* "public:  void    __cdecl ia32::context_t::restore(void)" */
    .extern _ZN4ia329context_t7restoreEv

    /* "private: void    __cdecl hvpp::vcpu_t::entry_host(void)" */
    .extern _ZN4hvpp6vcpu_t10entry_hostEv

    /* "private: void    __cdecl hvpp::vcpu_t::entry_guest(void)" */
    .extern _ZN4hvpp6vcpu_t11entry_guestEv

/*
 * private:
 *   static void __cdecl
 *   hvpp::vcpu_t::entry_guest_(void)
 *
 * Routine description:
 *
 *   Determines virtual cpu context from the stack pointer and calls
 *   vcpu_t::entry_guest() method.
 *
 */

.globl _ZN4hvpp6vcpu_t12entry_guest_Ev
    _ZN4hvpp6vcpu_t12entry_guest_Ev:

/*
 * RCX = &vcpu
 * RBX = &vcpu.launch_context_
 */
        leaq    VCPU_OFFSET(%rsp), %rcx
        leaq    VCPU_LAUNCH_CONTEXT_OFFSET(%rsp), %rbx

/*
 * Create shadow space
 */
        subq    $SHADOW_SPACE, %rsp
        call    _ZN4hvpp6vcpu_t11entry_guestEv

/*
 * Restore CPU context
 * Note that RBX is preserved, because it is non-volatile register
 */
        movq    %rbx, %rcx
        jmp     _ZN4ia329context_t7restoreEv

/*
 * private:
 *   static void __cdecl
 *   hvpp::vcpu_t::entry_host_(void)
 *
 * Routine description:
 *
 *   This method captures current CPU context and calls vcpu_t::entry_host()
 *   method.
 *
 */

.globl _ZN4hvpp6vcpu_t11entry_host_Ev
    _ZN4hvpp6vcpu_t11entry_host_Ev:
        pushq   %rcx

/*
 * RCX = &vcpu.exit_context_
 */
        leaq    (VCPU_EXIT_CONTEXT_OFFSET + 8)(%rsp), %rcx
        call    _ZN4ia329context_t7captureEv

/*
 * RBX = &vcpu.exit_context_
 * RCX = original value of RCX
 * RSP = original value of RSP
 */
        movq    %rcx, %rbx
        popq    %rcx

        movq    %rcx, CTX_OFF_RCX(%rbx)
        movq    %rsp, CTX_OFF_RSP(%rbx)

/*
 * RCX = &vcpu
 */
        leaq    VCPU_OFFSET(%rsp), %rcx

/*
 * Create shadow space
 */
        subq    $SHADOW_SPACE, %rsp
        call    _ZN4hvpp6vcpu_t10entry_hostEv

/*
 * Restore CPU context
 * Note that RBX is preserved, because it is non-volatile register
 */
        movq    %rbx, %rcx
        jmp     _ZN4ia329context_t7restoreEv
