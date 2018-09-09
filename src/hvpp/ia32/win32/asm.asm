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
;   Contains functions to access Intel CPU instructions which are not exported
;   by intrin.h.
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

    VMX_ERROR_CODE_SUCCESS              = 0
    VMX_ERROR_CODE_FAILED_WITH_STATUS   = 1
    VMX_ERROR_CODE_FAILED               = 2

    ;
    ; Pause/halt.
    ;

    ia32_asm_halt PROC
        hlt
        ret
    ia32_asm_halt ENDP

    ;
    ; Segment registers.
    ;

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

    ia32_asm_read_ar PROC
        lar     rax, rcx
        ret
    ia32_asm_read_ar ENDP

    ia32_asm_read_sl PROC
        lsl     eax, ecx
        ret
    ia32_asm_read_sl ENDP

    ;
    ; Descriptor registers.
    ;

    ia32_asm_read_gdtr PROC
        sgdt    fword ptr [rcx]
        ret
    ia32_asm_read_gdtr ENDP

    ia32_asm_write_gdtr PROC
        lgdt    fword ptr [rcx]
        ret
    ia32_asm_write_gdtr ENDP

    ;
    ; Control registers.
    ;

    ia32_asm_write_msw PROC
        lmsw    cx
        ret
    ia32_asm_write_msw ENDP

    ;
    ; Cache control
    ;

    ia32_asm_invd PROC
        invd
        ret
    ia32_asm_invd ENDP

    ;
    ; VMX.
    ;

    ia32_asm_vmx_vmcall PROC
        vmcall
        ret
    ia32_asm_vmx_vmcall ENDP

    ia32_asm_inv_ept PROC
        invept  rcx, oword ptr [rdx]
        jz      @jz
        jc      @jc
        xor     rax, rax
        ret

@jz:    mov     rax, VMX_ERROR_CODE_FAILED_WITH_STATUS
        ret

@jc:    mov     rax, VMX_ERROR_CODE_FAILED
        ret
    ia32_asm_inv_ept ENDP

    ia32_asm_inv_vpid PROC
        invvpid rcx, oword ptr [rdx]
        jz      @jz
        jc      @jc
        xor     rax, rax
        ret

@jz:    mov     rax, VMX_ERROR_CODE_FAILED_WITH_STATUS
        ret

@jc:    mov     rax, VMX_ERROR_CODE_FAILED
        ret
    ia32_asm_inv_vpid ENDP

END
