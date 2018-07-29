;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   asm.asm
;
; Abstract:
;
;   Contains functions to access Intel CPU instructions which are not
;   exported by intrin.h.
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

.CODE

    ia32_asm_read_cs PROC
        mov    ax, cs
        ret
    ia32_asm_read_cs ENDP

    ia32_asm_write_cs PROC
        mov    cs, cx
        ret
    ia32_asm_write_cs ENDP

    ia32_asm_read_ds PROC
        mov    ax, ds
        ret
    ia32_asm_read_ds ENDP

    ia32_asm_write_ds PROC
        mov    ds, cx
        ret
    ia32_asm_write_ds ENDP

    ia32_asm_read_es PROC
        mov    ax, es
        ret
    ia32_asm_read_es ENDP

    ia32_asm_write_es PROC
        mov    es, cx
        ret
    ia32_asm_write_es ENDP

    ia32_asm_read_fs PROC
        mov    ax, fs
        ret
    ia32_asm_read_fs ENDP

    ia32_asm_write_fs PROC
        mov    fs, cx
        ret
    ia32_asm_write_fs ENDP

    ia32_asm_read_gs PROC
        mov    ax, gs
        ret
    ia32_asm_read_gs ENDP

    ia32_asm_write_gs PROC
        mov    gs, cx
        ret
    ia32_asm_write_gs ENDP

    ia32_asm_read_ss PROC
        mov    ax, ss
        ret
    ia32_asm_read_ss ENDP

    ia32_asm_write_ss PROC
        mov    ss, cx
        ret
    ia32_asm_write_ss ENDP

    ;                                   ;
    ; --------------------------------- ;
    ;                                   ;

    ia32_asm_read_tr PROC
        str     ax
        ret
    ia32_asm_read_tr ENDP

    ia32_asm_write_tr PROC
        ltr     cx
        ret
    ia32_asm_write_tr ENDP

    ia32_asm_read_ldtr PROC
        sldt    ax
        ret
    ia32_asm_read_ldtr ENDP

    ia32_asm_write_ldtr PROC
        lldt    cx
        ret
    ia32_asm_write_ldtr ENDP

    ;                                   ;
    ; --------------------------------- ;
    ;                                   ;

    ia32_asm_read_ar PROC
        lar     rax, rcx
        ret
    ia32_asm_read_ar ENDP

    ia32_asm_read_sl PROC
        lsl     eax, ecx
        ret
    ia32_asm_read_sl ENDP

    ia32_asm_read_gdtr PROC
        sgdt    fword ptr [rcx]
        ret
    ia32_asm_read_gdtr ENDP

    ia32_asm_write_gdtr PROC
        lgdt    fword ptr [rcx]
        ret
    ia32_asm_write_gdtr ENDP

    ;                                   ;
    ; --------------------------------- ;
    ;                                   ;

    ia32_asm_write_msw PROC
        lmsw ax
        ret
    ia32_asm_write_msw ENDP

    ia32_asm_invd PROC
        invd
        ret
    ia32_asm_invd ENDP

    ;                                   ;
    ; --------------------------------- ;
    ;                                   ;

    ia32_asm_vmx_vmcall PROC
        vmcall
        ret
    ia32_asm_vmx_vmcall ENDP

    ;                                   ;
    ; --------------------------------- ;
    ;                                   ;

    ia32_asm_inv_ept PROC
        invept  rcx, oword ptr [rdx]
        ret
    ia32_asm_inv_ept ENDP

    ia32_asm_inv_vpid PROC
        invvpid rcx, oword ptr [rdx]
        ret
    ia32_asm_inv_vpid ENDP


    ia32_asm_in_dword_ex PROC
        push rbx
        push r12
        push r13

        ;
        ; Current status:
        ;   rcx      = port
        ;   rdx      = &rax
        ;   r8       = &rbx
        ;   r9       = &rcx
        ;   [rsp+64] = &rdx
        ;

        mov r10, rdx
        mov r11, r8
        mov r12, r9
        mov r13, qword ptr[rsp + 64]

        mov rdx, rcx

        ;
        ; Current status:
        ;   r9  = &rax
        ;   r10 = &rbx
        ;   r11 = &rcx
        ;   r12 = &rdx
        ;   rdx = port
        ;

        mov rax, qword ptr[r10]
        mov rbx, qword ptr[r11]
        mov rcx, qword ptr[r12]
        mov rdx, qword ptr[r13]

        ;
        ; Perform the "in" instruction.
        ;
        in eax, dx

        ;
        ; Save RAX, RBX, RCX and RDX.
        ;
        mov qword ptr[r10], rax
        mov qword ptr[r11], rbx
        mov qword ptr[r12], rcx
        mov qword ptr[r13], rdx

        pop r13
        pop r12
        pop rbx
        ret
    ia32_asm_in_dword_ex ENDP

    ia32_asm_out_dword_ex PROC
        push rbx
        push r12
        push r13

        ;
        ; Current status:
        ;   rcx      = port
        ;   rdx      = &rax
        ;   r8       = &rbx
        ;   r9       = &rcx
        ;   [rsp+64] = &rdx
        ;

        mov r10, rdx
        mov r11, r8
        mov r12, r9
        mov r13, qword ptr[rsp + 64]

        mov rdx, rcx

        ;
        ; Current status:
        ;   r9  = &rax
        ;   r10 = &rbx
        ;   r11 = &rcx
        ;   r12 = &rdx
        ;   rdx = port
        ;

        mov rax, qword ptr[r10]
        mov rbx, qword ptr[r11]
        mov rcx, qword ptr[r12]
        mov rdx, qword ptr[r13]

        ;
        ; Perform the "out" instruction.
        ;
        out dx, eax

        ;
        ; Save RAX, RBX, RCX and RDX.
        ;
        mov qword ptr[r10], rax
        mov qword ptr[r11], rbx
        mov qword ptr[r12], rcx
        mov qword ptr[r13], rdx

        pop r13
        pop r12
        pop rbx
        ret
    ia32_asm_out_dword_ex ENDP


END
