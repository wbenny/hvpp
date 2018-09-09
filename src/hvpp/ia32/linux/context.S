#include "ia32/linux/common.S"

.globl _ZN4ia329context_t7captureEv
    _ZN4ia329context_t7captureEv:
        pushfq
        movq    %rax, CTX_OFF_RAX(%rdi)
        movq    %rcx, CTX_OFF_RCX(%rdi)
        movq    %rdx, CTX_OFF_RDX(%rdi)
        movq    %rbx, CTX_OFF_RBX(%rdi)
        movq    %rbp, CTX_OFF_RBP(%rdi)
        movq    %rsi, CTX_OFF_RSI(%rdi)
        movq    %rdi, CTX_OFF_RDI(%rdi)
        movq    %r8,  CTX_OFF_R8(%rdi)
        movq    %r9,  CTX_OFF_R9(%rdi)
        movq    %r10, CTX_OFF_R10(%rdi)
        movq    %r11, CTX_OFF_R11(%rdi)
        movq    %r12, CTX_OFF_R12(%rdi)
        movq    %r13, CTX_OFF_R13(%rdi)
        movq    %r14, CTX_OFF_R14(%rdi)
        movq    %r15, CTX_OFF_R15(%rdi)

/*
 * RSP, RIP and RFLAGS are captured here.
 */
        leaq    16(%rsp), %rax
        mov     %rax, CTX_OFF_RSP(%rdi)

        movq    8(%rsp), %rax
        movq    %rax, CTX_OFF_RIP(%rdi)

        movq    (%rsp), %rax
        movq    %rax, CTX_OFF_RFLAGS(%rdi)

        xorq    %rax, %rax
        addq    $8, %rsp
        ret

.globl _ZN4ia329context_t7restoreEv
    _ZN4ia329context_t7restoreEv:
/*
 * We use shadow space of the caller + additional 8 bytes
 * for the IRETQ context.  Note that we can use the callers
 * shadow space without any worries, because this function
 * never returns to the caller.
 */
        subq    $8, %rsp

/*
 * Build iretq context (SS, RSP, RFLAGS, CS, RIP).
 */
        movw    %ss, 32(%rsp)

        movq    CTX_OFF_RSP(%rdi), %rax
        movq    %rax, 24(%rsp)

        movq    CTX_OFF_RFLAGS(%rdi), %rax
        movq    %rax, 16(%rsp)

        movw    %cs, 8(%rsp)

        movq    CTX_OFF_RIP(%rdi), %rax
        movq    %rax, (%rsp)

        movq    CTX_OFF_RAX(%rdi), %rax
        movq    CTX_OFF_RCX(%rdi), %rcx
        movq    CTX_OFF_RDX(%rdi), %rdx
        movq    CTX_OFF_RBX(%rdi), %rbx
        movq    CTX_OFF_RBP(%rdi), %rbp
        movq    CTX_OFF_RSI(%rdi), %rsi
        movq    CTX_OFF_R8(%rdi),  %r8
        movq    CTX_OFF_R9(%rdi),  %r9
        movq    CTX_OFF_R10(%rdi), %r10
        movq    CTX_OFF_R11(%rdi), %r11
        movq    CTX_OFF_R12(%rdi), %r12
        movq    CTX_OFF_R13(%rdi), %r13
        movq    CTX_OFF_R14(%rdi), %r14
        movq    CTX_OFF_R15(%rdi), %r15

/*
 * Restore RDI at the end.
 */
        movq    CTX_OFF_RDI(%rdi), %rdi
        iretq
