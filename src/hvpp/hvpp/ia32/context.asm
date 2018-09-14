;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   context.asm
;
; Abstract:
;
;   This module implements AMD64-specific code to capture and restore
;   the context of the caller.
;
; Author:
;
;    Petr Benes (@PetrBenes) 26-Jul-2018 - Initial version
;
; Environment:
;
;    Kernel mode only.
;
;--

INCLUDE ksamd64.inc
INCLUDE hvpp/ia32/common.inc

.CODE

;++
;
; public:
;   int __cdecl
;   ia32::context_t::capture(void)
;
; Routine description:
;
;   This method captures the context of the caller.
;
; Return Value:
;
;   0 if caller captured the context, RAX otherwise.
;
;--

    ?capture@context_t@ia32@@QEAAHXZ PROC
        pushfq
        mov     context_t.$rax[rcx], rax
        mov     context_t.$rcx[rcx], rcx
        mov     context_t.$rdx[rcx], rdx
        mov     context_t.$rbx[rcx], rbx
        mov     context_t.$rbp[rcx], rbp
        mov     context_t.$rsi[rcx], rsi
        mov     context_t.$rdi[rcx], rdi
        mov     context_t.$r8 [rcx], r8
        mov     context_t.$r9 [rcx], r9
        mov     context_t.$r10[rcx], r10
        mov     context_t.$r11[rcx], r11
        mov     context_t.$r12[rcx], r12
        mov     context_t.$r13[rcx], r13
        mov     context_t.$r14[rcx], r14
        mov     context_t.$r15[rcx], r15

;
; RSP, RIP and RFLAGS are captured here.
;
        lea     rax, qword ptr [rsp + 16]
        mov     context_t.$rsp[rcx], rax

        mov     rax, qword ptr [rsp +  8]
        mov     context_t.$rip[rcx], rax

        mov     rax, qword ptr [rsp +  0]
        mov     context_t.$rflags[rcx], rax

        xor     rax, rax
        add     rsp, 8
        ret
    ?capture@context_t@ia32@@QEAAHXZ ENDP

;++
;
; public:
;   void __cdecl
;   ia32::context_t::restore(void)
;
; Routine description:
;
;   This method restores the context of the caller to the specified
;   context.
;
; Return Value:
;
;   None - there is no return from this method.
;
;--

    ?restore@context_t@ia32@@QEAAXXZ PROC
;
; We use shadow space of the caller + additional 8 bytes
; for the IRETQ context.  Note that we can use the callers
; shadow space without any worries, because this function
; never returns to the caller.
;
        sub     rsp, 8

;
; Build iretq context (SS, RSP, RFLAGS, CS, RIP).
;
        mov     word ptr [rsp + 4 * 8], ss

        mov     rax, context_t.$rsp[rcx]
        mov     qword ptr [rsp + 3 * 8], rax

        mov     rax, context_t.$rflags[rcx]
        mov     qword ptr [rsp + 2 * 8], rax

        mov     word ptr [rsp + 1 * 8], cs

        mov     rax, context_t.$rip[rcx]
        mov     qword ptr [rsp + 0 * 8], rax

        mov     rax, context_t.$rax[rcx]
        mov     rdx, context_t.$rdx[rcx]
        mov     rbx, context_t.$rbx[rcx]
        mov     rbp, context_t.$rbp[rcx]
        mov     rsi, context_t.$rsi[rcx]
        mov     rdi, context_t.$rdi[rcx]
        mov     r8 , context_t.$r8 [rcx]
        mov     r9 , context_t.$r9 [rcx]
        mov     r10, context_t.$r10[rcx]
        mov     r11, context_t.$r11[rcx]
        mov     r12, context_t.$r12[rcx]
        mov     r13, context_t.$r13[rcx]
        mov     r14, context_t.$r14[rcx]
        mov     r15, context_t.$r15[rcx]

;
; Restore RCX at the end.
;
        mov     rcx, context_t.$rcx[rcx]
        iretq
    ?restore@context_t@ia32@@QEAAXXZ ENDP

END
