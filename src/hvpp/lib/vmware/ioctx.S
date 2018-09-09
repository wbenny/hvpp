#include "ia32/linux/common.S"

/*
 * Definitions for ia32::vmx::exit_qualification_io_instruction_t.
 */
#define EQ_IO_SIZE_OF_ACCESS_MASK           0x7                 /* bits 0-2 */
#define EQ_IO_ACCESS_TYPE_MASK              0x8                 /* bit  3 */
#define EQ_IO_ACCESS_TYPE_SHIFT             3
#define EQ_IO_STRING_INST_MASK              0x10                /* bit  4 */
#define EQ_IO_STRING_INST_SHIFT             4
#define EQ_IO_REP_PREFIXED_MASK             0x20                /* bit  5 */
#define EQ_IO_REP_PREFIXED_SHIFT            5
#define EQ_IO_PORT_MASK                     0xffffffffffff0000  /* bits 16-31 */
#define EQ_IO_PORT_SHIFT                    16

#define RFLAGS_DF_MASK                      0x400               /* bit 10 */

/*
 * int
 * ia32_asm_io_with_context(
 *   ia32::vmx::exit_qualification_io_instruction_t exit_qualification,
 *   ia32::context_t* context
 *   )
 *
 * Routine description:
 *
 *   This function performs in or out instruction with provided CPU context.
 *   If the function succeeds, the CPU context is updated with the returned
 *   register state after the performed instruction.
 *
 *   This function assumes the port is provided in DX register of the CPU
 *   context (NOT immediate).  If the port and DX differ, this function
 *   fails and returns 0.
 *
 *   Note that because the function updates CPU context after the instruction
 *   is done, the RCX should be automatically reset to 0 if rep_prefixed == 1.
 *
 *   Pseudocode:
 *     LOAD from CPU context: rax, rbx, rcx, rdx, rsi, rdi, rbp
 *
 *     if (exit_qualification.string_instruction == 1) {
 *       if (exit_qualification.rep_prefixed == 1) {
 *         rep ins or rep outs is called
 *         with respect to the requested size in exit_qualification.size_of_access
 *       } else {
 *         ins or outs is called
 *         with respect to the requested size in exit_qualification.size_of_access
 *       }
 *     } else {
 *       in or out is called
 *       with respect to the requested size in exit_qualification.size_of_access
 *     }
 *
 *     STORE to CPU context: rax, rbx, rcx, rdx, rsi, rdi, rbp
 *
 *   #TODO: What about RFLAGS?
 *
 * Arguments:
 *
 *   exit_qualification (rcx) - VMCS Exit qualification field for I/O
 *                              instructions.
 *
 *   context (rdx) - Pointer to the CPU context on which the I/O instruction
 *                   will be performed.
 *
 * Return Value:
 *
 *   Returns 1 if the operation succeeded
 */
.globl ia32_asm_io_with_context
    ia32_asm_io_with_context:
        pushq   %rbp
        pushq   %rbx

/*
 * Map input parameters.
 *   R10 = exit_qualification
 *   R11 = context
 */
        movq    %rdi, %r10
        movq    %rsi, %r11

/*
 * Restore context partially.
 */
        movq    CTX_OFF_RAX(%r11), %rax
        movq    CTX_OFF_RCX(%r11), %rcx
        movq    CTX_OFF_RDX(%r11), %rdx
        movq    CTX_OFF_RBX(%r11), %rbx
        movq    CTX_OFF_RBP(%r11), %rbp
        movq    CTX_OFF_RSI(%r11), %rsi
        movq    CTX_OFF_RDI(%r11), %rdi
        movq    CTX_OFF_RBP(%r11), %rbp

/*
 * Set DF (direction flag) according to the DF in the CPU context.
 */
        movq    CTX_OFF_RFLAGS(%r11), %r8
        andq    $RFLAGS_DF_MASK, %r8
        testq   %r8, %r8
        jnz     l_std

l_cld:  cld
        jmp     prtchk                  /* DF cleared, skip the STD and jump to port-check. */

l_std:  std

/*
 * Lower word of RDX (the DX part) must equal to the port provided in the
 * exit qualification.  This is because in/out instruction either accepts
 * port number as immediate (e.g.: in eax, 0x1234) or only in DX (e.g.:
 * in eax, dx).  This also applies for ins/outs instructions, which operate
 * with strings.
 *
 * if ((context.rdx & 0xffff) != exit_qualification.port)
 *   goto err;
 */
prtchk: movq    %r10, %r8
        andq    $EQ_IO_PORT_MASK, %r8
        shrq    $EQ_IO_PORT_SHIFT, %r8
        cmp     %r8w, %dx
        jne     err

/*
 * r8 = exit_qualification.size_of_access + 1
 */
        movq    %r10, %r8
        andq    $EQ_IO_SIZE_OF_ACCESS_MASK, %r8
        inc     %r8

/*
 * if (exit_qualification.access_type == access_out { 0 })
 *   goto l_out;
 */
        testq   $EQ_IO_ACCESS_TYPE_MASK, %r10
        jz      l_out

/*
 * if (exit_qualification.string_instruction)
 *   goto l_ins;
 */
        testq   $EQ_IO_STRING_INST_MASK, %r10
        jnz     l_ins

/*
 * Test for "in [single value]" instruction.
 *
 * switch (r8 { size }) {
 *   case 1:  goto in1;
 *   case 2:  goto in2;
 *   case 4:  goto in4;
 *   default: goto err;
 * }
 */
l_in:   cmp     $1, %r8
        je      in1
        cmp     $2, %r8
        je      in2
        cmp     $4, %r8
        je      in4
        jmp     err

/*
 * Test for "in [string of values]" instruction.
 *
 * if (!exit_qualification.rep_prefixed) {
 *   switch (size) {
 *     case 1:  goto ins1;
 *     case 2:  goto ins2;
 *     case 4:  goto ins4;
 *     default: goto err;
 *   }
 * } else {
 *   switch (size) {
 *     case 1:  goto insr1;
 *     case 2:  goto insr2;
 *     case 4:  goto insr4;
 *     default: goto err;
 *   }
 * }
 */
l_ins:  test    $EQ_IO_REP_PREFIXED_MASK, %r10
        jnz     l_insr

        cmp     $1, %r8
        je      ins1
        cmp     $2, %r8
        je      ins2
        cmp     $4, %r8
        je      ins4
        jmp     err

l_insr: cmp     $1, %r8
        je      insr1
        cmp     $2, %r8
        je      insr2
        cmp     $4, %r8
        je      insr4
        jmp     err

/*
 * if (exit_qualification.string_instruction)
 *   goto l_outs;
 */
l_out:  test    $EQ_IO_STRING_INST_MASK, %r10
        jnz     l_outs

/*
 * Test for "out [single value]" instruction.
 *
 * switch (size) {
 *   case 1:  goto out1;
 *   case 2:  goto out2;
 *   case 4:  goto out4;
 *   default: goto err;
 * }
 */
        cmp     $1, %r8
        je      out1
        cmp     $2, %r8
        je      out2
        cmp     $4, %r8
        je      out4
        jmp     err

/*
 * Test for "out [string of values]" instruction.
 *
 * if (!exit_qualification.rep_prefixed) {
 *   switch (size) {
 *     case 1:  goto outs1;
 *     case 2:  goto outs2;
 *     case 4:  goto outs4;
 *     default: goto err;
 *   }
 * } else {
 *   switch (size) {
 *     case 1:  goto outsr1;
 *     case 2:  goto outsr2;
 *     case 4:  goto outsr4;
 *     default: goto err;
 *   }
 * }
 */
l_outs: test    $EQ_IO_REP_PREFIXED_MASK, %r10
        jnz     l_outsr

        cmp     $1, %r8
        je      outs1
        cmp     $2, %r8
        je      outs2
        cmp     $4, %r8
        je      outs4
        jmp     err

l_outsr:cmp     $1, %r8
        je      outsr1
        cmp     $2, %r8
        je      outsr2
        cmp     $4, %r8
        je      outsr4
        jmp     err

/*
 * Perform the desired instruction.
 */
in1:    in      %dx, %al
        jmp     exit
in2:    in      %dx, %ax
        jmp     exit
in4:    in      %dx, %eax
        jmp     exit
ins1:   insb
        jmp     exit
ins2:   insw
        jmp     exit
ins4:   insl
        jmp     exit
insr1:  rep     insb
        jmp     exit
insr2:  rep     insw
        jmp     exit
insr4:  rep     insl
        jmp     exit
out1:   out     %al, %dx
        jmp     exit
out2:   out     %ax, %dx
        jmp     exit
out4:   out     %eax, %dx
        jmp     exit
outs1:  outsb
        jmp     exit
outs2:  outsw
        jmp     exit
outs4:  outsl
        jmp     exit
outsr1: rep     outsb
        jmp     exit
outsr2: rep     outsw
        jmp     exit
outsr4: rep     outsl

/*
 * Restore partial context.
 */
exit:   movq    %rax, CTX_OFF_RAX(%r11)
        movq    %rcx, CTX_OFF_RCX(%r11)
        movq    %rdx, CTX_OFF_RDX(%r11)
        movq    %rbx, CTX_OFF_RBX(%r11)
        movq    %rbp, CTX_OFF_RBP(%r11)
        movq    %rsi, CTX_OFF_RSI(%r11)
        movq    %rdi, CTX_OFF_RDI(%r11)
        movq    %rbp, CTX_OFF_RBP(%r11)

/*
 * Signalize success by returning 1, and return.
 */
        xorq    %rax, %rax
        incq    %rax
        jmp     noerr
/*
 * Restore non-volatile registers, and return.
 */
err:    xorl    %eax, %eax
noerr:  popq    %rbx
        popq    %rbp
        ret
