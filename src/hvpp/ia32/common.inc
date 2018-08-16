;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   common.inc
;
; Abstract:
;
;   Contains commonly used constants, structures and macros.
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

    context_t struct
        $rax    dq ?
        $rcx    dq ?
        $rdx    dq ?
        $rbx    dq ?
        $rsp    dq ?
        $rbp    dq ?
        $rsi    dq ?
        $rdi    dq ?
        $r8     dq ?
        $r9     dq ?
        $r10    dq ?
        $r11    dq ?
        $r12    dq ?
        $r13    dq ?
        $r14    dq ?
        $r15    dq ?
        $rip    dq ?
        $rflags dq ?
    context_t ends
