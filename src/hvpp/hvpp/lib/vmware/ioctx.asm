;++
;
; Copyright (c) Petr Benes. All rights reserved.
;
; Module:
;
;   ioctx.asm
;
; Abstract:
;
;   TODO.
;
; Author:
;
;    Petr Benes (@PetrBenes) 06-Aug-2018 - Initial version
;
; Environment:
;
;    Kernel mode only.
;
;--

INCLUDE ksamd64.inc
INCLUDE hvpp/ia32/common.inc

.CODE

;
; Definitions for ia32::vmx::exit_qualification_io_instruction_t.
;
    EQ_IO_SIZE_OF_ACCESS_MASK           = 7h                ; bits 0-2
    EQ_IO_ACCESS_TYPE_MASK              = 8h                ; bit  3
    EQ_IO_ACCESS_TYPE_SHIFT             = 3
    EQ_IO_STRING_INST_MASK              = 10h               ; bit  4
    EQ_IO_STRING_INST_SHIFT             = 4
    EQ_IO_REP_PREFIXED_MASK             = 20h               ; bit  5
    EQ_IO_REP_PREFIXED_SHIFT            = 5
    EQ_IO_PORT_MASK                     = 0ffff0000h        ; bits 16-31
    EQ_IO_PORT_SHIFT                    = 16

    RFLAGS_DF_MASK                      = 0400h             ; bit 10

;++
;
; int
; ia32_asm_io_with_context(
;   ia32::vmx::exit_qualification_io_instruction_t exit_qualification,
;   ia32::context_t* context
;   )
;
; Routine description:
;
;   This function performs in or out instruction with provided CPU context.
;   If the function succeeds, the CPU context is updated with the returned
;   register state after the performed instruction.
;
;   This function assumes the port is provided in DX register of the CPU
;   context (NOT immediate).  If the port and DX differ, this function
;   fails and returns 0.
;
;   Note that because the function updates CPU context after the instruction
;   is done, the RCX should be automatically reset to 0 if rep_prefixed == 1.
;
;   Pseudocode:
;     LOAD from CPU context: rax, rbx, rcx, rdx, rsi, rdi, rbp
;
;     if (exit_qualification.string_instruction == 1) {
;       if (exit_qualification.rep_prefixed == 1) {
;         rep ins or rep outs is called
;         with respect to the requested size in exit_qualification.size_of_access
;       } else {
;         ins or outs is called
;         with respect to the requested size in exit_qualification.size_of_access
;       }
;     } else {
;       in or out is called
;       with respect to the requested size in exit_qualification.size_of_access
;     }
;
;     STORE to CPU context: rax, rbx, rcx, rdx, rsi, rdi, rbp
;
;   #TODO: What about RFLAGS?
;
; Arguments:
;
;   exit_qualification (rcx) - VMCS Exit qualification field for I/O
;                              instructions.
;
;   context (rdx) - Pointer to the CPU context on which the I/O instruction
;                   will be performed.
;
; Return Value:
;
;   Returns 1 if the operation succeeded
;
;--
    ia32_asm_io_with_context PROC
        push    rbp
        push    rbx
        push    rsi
        push    rdi

;
; Map input parameters.
;   R10 = exit_qualification
;   R11 = context
;
        mov     r10, rcx
        mov     r11, rdx

;
; Restore context partially.
;
        mov     rax, context_t.$rax[r11]
        mov     rcx, context_t.$rcx[r11]
        mov     rdx, context_t.$rdx[r11]
        mov     rbx, context_t.$rbx[r11]
        mov     rbp, context_t.$rbp[r11]
        mov     rsi, context_t.$rsi[r11]
        mov     rdi, context_t.$rdi[r11]
        mov     rbp, context_t.$rbp[r11]

;
; Set DF (direction flag) according to the DF in the CPU context.
;
        mov     r8, context_t.$rflags[r11]
        and     r8, RFLAGS_DF_MASK
        test    r8, r8
        jnz     @std

@cld:   cld
        jmp     prtchk                  ; DF cleared, skip the STD and jump to port-check.

@std:   std

;
; Lower word of RDX (the DX part) must equal to the port provided in the
; exit qualification.  This is because in/out instruction either accepts
; port number as immediate (e.g.: in eax, 0x1234) or only in DX (e.g.:
; in eax, dx).  This also applies for ins/outs instructions, which operate
; with strings.
;
; if ((context.rdx & 0xffff) != exit_qualification.port)
;   goto err;
;
prtchk: mov     r8, r10
        and     r8, EQ_IO_PORT_MASK
        shr     r8, EQ_IO_PORT_SHIFT
        cmp     dx, r8w
        jne     err

;
; r8 = exit_qualification.size_of_access + 1
;
        mov     r8, r10
        and     r8, EQ_IO_SIZE_OF_ACCESS_MASK
        inc     r8

;
; if (exit_qualification.access_type == access_out /* 0 */)
;   goto @out;
;
        test    r10, EQ_IO_ACCESS_TYPE_MASK
        jz      @out

;
; if (exit_qualification.string_instruction)
;   goto @ins;
;
        test    r10, EQ_IO_STRING_INST_MASK
        jnz     @ins

;
; Test for "in [single value]" instruction.
;
; switch (r8 /* size */) {
;   case 1:  goto in1;
;   case 2:  goto in2;
;   case 4:  goto in4;
;   default: goto err;
; }
;
@in:    cmp     r8, 1
        je      in1
        cmp     r8, 2
        je      in2
        cmp     r8, 4
        je      in4
        jmp     err

;
; Test for "in [string of values]" instruction.
;
; if (!exit_qualification.rep_prefixed) {
;   switch (size) {
;     case 1:  goto ins1;
;     case 2:  goto ins2;
;     case 4:  goto ins4;
;     default: goto err;
;   }
; } else {
;   switch (size) {
;     case 1:  goto insr1;
;     case 2:  goto insr2;
;     case 4:  goto insr4;
;     default: goto err;
;   }
; }
;
@ins:   test    r10, EQ_IO_REP_PREFIXED_MASK
        jnz     @insr

        cmp     r8, 1
        je      ins1
        cmp     r8, 2
        je      ins2
        cmp     r8, 4
        je      ins4
        jmp     err

@insr:  cmp     r8, 1
        je      insr1
        cmp     r8, 2
        je      insr2
        cmp     r8, 4
        je      insr4
        jmp     err

;
; if (exit_qualification.string_instruction)
;   goto @outs;
;
@out:   test    r10, EQ_IO_STRING_INST_MASK
        jnz     @outs

;
; Test for "out [single value]" instruction.
;
; switch (size) {
;   case 1:  goto out1;
;   case 2:  goto out2;
;   case 4:  goto out4;
;   default: goto err;
; }
;
        cmp     r8, 1
        je      out1
        cmp     r8, 2
        je      out2
        cmp     r8, 4
        je      out4
        jmp     err

;
; Test for "out [string of values]" instruction.
;
; if (!exit_qualification.rep_prefixed) {
;   switch (size) {
;     case 1:  goto outs1;
;     case 2:  goto outs2;
;     case 4:  goto outs4;
;     default: goto err;
;   }
; } else {
;   switch (size) {
;     case 1:  goto outsr1;
;     case 2:  goto outsr2;
;     case 4:  goto outsr4;
;     default: goto err;
;   }
; }
;
@outs:  test    r10, EQ_IO_REP_PREFIXED_MASK
        jnz     @outsr

        cmp     r8, 1
        je      outs1
        cmp     r8, 2
        je      outs2
        cmp     r8, 4
        je      outs4
        jmp     err

@outsr: cmp     r8, 1
        je      outsr1
        cmp     r8, 2
        je      outsr2
        cmp     r8, 4
        je      outsr4
        jmp     err

;
; Perform the desired instruction.
;
in1:    in      al, dx
        jmp     exit
in2:    in      ax, dx
        jmp     exit
in4:    in      eax, dx
        jmp     exit
ins1:   insb
        jmp     exit
ins2:   insw
        jmp     exit
ins4:   insd
        jmp     exit
insr1:  rep     insb
        jmp     exit
insr2:  rep     insw
        jmp     exit
insr4:  rep     insd
        jmp     exit
out1:   out     dx, al
        jmp     exit
out2:   out     dx, ax
        jmp     exit
out4:   out     dx, eax
        jmp     exit
outs1:  outsb
        jmp     exit
outs2:  outsw
        jmp     exit
outs4:  outsd
        jmp     exit
outsr1: rep     outsb
        jmp     exit
outsr2: rep     outsw
        jmp     exit
outsr4: rep     outsd

;
; Restore partial context.
;
exit:   mov     context_t.$rax[r11], rax
        mov     context_t.$rcx[r11], rcx
        mov     context_t.$rdx[r11], rdx
        mov     context_t.$rbx[r11], rbx
        mov     context_t.$rbp[r11], rbp
        mov     context_t.$rsi[r11], rsi
        mov     context_t.$rdi[r11], rdi
        mov     context_t.$rbp[r11], rbp

;
; Signalize success by returning 1, and return.
;
        xor     rax, rax
        inc     rax
        jmp     noerr
;
; Restore non-volatile registers, and return.
;
err:    xor     eax, eax
noerr:  pop     rdi
        pop     rsi
        pop     rbx
        pop     rbp
        ret
    ia32_asm_io_with_context ENDP

END
