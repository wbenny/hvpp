;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   vcpu.asm
;
; Abstract:
;
;   This module implements VM-Exit stub handler.
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
INCLUDE ia32/common.inc

.CODE

;
; Useful definitions.
;
    VCPU_OFFSET                = -8000h             ; -vcpu_stack_size
    VCPU_LAUNCH_CONTEXT_OFFSET =  0
    VCPU_EXIT_CONTEXT_OFFSET   =  144               ; sizeof context
    SHADOW_SPACE               =  20h

;
; Externally used symbols.
;
    ; "public:  __int64 __cdecl ia32::context_t::capture(void)"
    EXTERN ?capture@context_t@ia32@@QEAAHXZ           : PROC

    ; "public:  void    __cdecl ia32::context_t::restore(void)"
    EXTERN ?restore@context_t@ia32@@QEAAXXZ           : PROC

    ; "private: void    __cdecl hvpp::vcpu_t::entry_host(void)"
    EXTERN ?entry_host@vcpu_t@hvpp@@AEAAXXZ           : PROC

    ; "private: void    __cdecl hvpp::vcpu_t::entry_guest(void)"
    EXTERN ?entry_guest@vcpu_t@hvpp@@AEAAXXZ          : PROC

;++
;
; private:
;   static void __cdecl
;   hvpp::vcpu_t::entry_guest_(void)
;
; Routine description:
;
;   Determines virtual cpu context from the stack pointer and calls
;
;--

    ?entry_guest_@vcpu_t@hvpp@@CAXXZ PROC
;
; RCX = &vcpu
; RBX = &vcpu.launch_context_
;
        lea     rcx, qword ptr [rsp + VCPU_OFFSET]
        lea     rbx, qword ptr [rsp + VCPU_LAUNCH_CONTEXT_OFFSET]

;
; Create shadow space
;
        sub     rsp, SHADOW_SPACE
        call    ?entry_guest@vcpu_t@hvpp@@AEAAXXZ

;
; Restore CPU context
; Note that RBX is preserved, because it is non-volatile register
;
        mov     rcx, rbx
        jmp     ?restore@context_t@ia32@@QEAAXXZ
    ?entry_guest_@vcpu_t@hvpp@@CAXXZ ENDP

;++
;
; private:
;   static void __cdecl
;   hvpp::vcpu_t::entry_host_(void)
;
; Routine description:
;
;   This method captures current CPU context and calls regular
;   VM-Exit handler.
;
;--

    ?entry_host_@vcpu_t@hvpp@@CAXXZ PROC
        push    rcx

;
; RCX = &vcpu.exit_context_
;
        lea     rcx, qword ptr [rsp + 8 + VCPU_EXIT_CONTEXT_OFFSET]
        call    ?capture@context_t@ia32@@QEAAHXZ

;
; RBX = &vcpu.exit_context_
; RCX = original value of RCX
; RSP = original value of RSP
;
        mov     rbx, rcx
        pop     rcx

        mov     context_t.$rcx[rbx], rcx
        mov     context_t.$rsp[rbx], rsp

;
; RCX = &vcpu
;
        lea     rcx, qword ptr [rsp + VCPU_OFFSET]

;
; Create shadow space
;
        sub     rsp, SHADOW_SPACE
        call    ?entry_host@vcpu_t@hvpp@@AEAAXXZ

;
; Restore CPU context
; Note that RBX is preserved, because it is non-volatile register
;
        mov     rcx, rbx
        jmp     ?restore@context_t@ia32@@QEAAXXZ
    ?entry_host_@vcpu_t@hvpp@@CAXXZ ENDP

END
