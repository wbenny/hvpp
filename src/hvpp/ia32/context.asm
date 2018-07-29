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
INCLUDE common.inc

.CODE

;++
;
; public:
;   int __cdecl
;   ia32::context::capture(void)
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

    ?capture@context@ia32@@QEAAHXZ PROC
        pushfq
        mov     context.$rax[rcx], rax
        mov     context.$rcx[rcx], rcx
        mov     context.$rdx[rcx], rdx
        mov     context.$rbx[rcx], rbx
        mov     context.$rbp[rcx], rbp
        mov     context.$rsi[rcx], rsi
        mov     context.$rdi[rcx], rdi
        mov     context.$r8 [rcx], r8
        mov     context.$r9 [rcx], r9
        mov     context.$r10[rcx], r10
        mov     context.$r11[rcx], r11
        mov     context.$r12[rcx], r12
        mov     context.$r13[rcx], r13
        mov     context.$r14[rcx], r14
        mov     context.$r15[rcx], r15

;
; RSP, RIP and RFLAGS are captured here.
;
        lea     rax, qword ptr [rsp + 16]
        mov     context.$rsp[rcx], rax

        mov     rax, qword ptr [rsp +  8]
        mov     context.$rip[rcx], rax

        mov     rax, qword ptr [rsp +  0]
        mov     context.$rflags[rcx], rax

        xor     rax, rax
        add     rsp, 8
        ret
    ?capture@context@ia32@@QEAAHXZ ENDP

;++
;
; public:
;   void __cdecl
;   ia32::context::restore(void)
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

    ?restore@context@ia32@@QEAAXXZ PROC
;
; We use shadow space of the caller + additional 8 bytes
; for the IRETQ context. Note that we can use the callers
; shadow space without any worries, because this function
; never returns to the caller.
;
        sub     rsp, 8

;
; Build iretq context (SS, RSP, RFLAGS, CS, RIP).
;
        mov     word ptr [rsp + 4 * 8], ss

        mov     rax, context.$rsp[rcx]
        mov     qword ptr [rsp + 3 * 8], rax

        mov     rax, context.$rflags[rcx]
        mov     qword ptr [rsp + 2 * 8], rax

        mov     word ptr [rsp + 1 * 8], cs

        mov     rax, context.$rip[rcx]
        mov     qword ptr [rsp + 0 * 8], rax

        mov     rax, context.$rax[rcx]
        mov     rdx, context.$rdx[rcx]
        mov     rbx, context.$rbx[rcx]
        mov     rbp, context.$rbp[rcx]
        mov     rsi, context.$rsi[rcx]
        mov     rdi, context.$rdi[rcx]
        mov     r8 , context.$r8 [rcx]
        mov     r9 , context.$r9 [rcx]
        mov     r10, context.$r10[rcx]
        mov     r11, context.$r11[rcx]
        mov     r12, context.$r12[rcx]
        mov     r13, context.$r13[rcx]
        mov     r14, context.$r14[rcx]
        mov     r15, context.$r15[rcx]

;
; Restore RCX at the end.
;
        mov     rcx, context.$rcx[rcx]
        iretq
    ?restore@context@ia32@@QEAAXXZ ENDP

END
