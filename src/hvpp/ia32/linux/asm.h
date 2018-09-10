#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

//
// Breakpoint.
//

inline void ia32_asm_int3() noexcept
{
  asm volatile(
    "int3"
    );
}

//
// CPUID.
//

inline void ia32_asm_cpuid(uint32_t result[4], uint32_t eax) noexcept
{
  asm volatile(
    "cpuid"
    : "=a" (result[0]), "=b" (result[1]), "=c" (result[2]), "=d" (result[3])
    :  "0" (eax)
    );
}

inline void ia32_asm_cpuid_ex(uint32_t result[4], uint32_t eax, uint32_t ecx) noexcept
{
  asm volatile(
    "cpuid"
    : "=a" (result[0]), "=b" (result[1]), "=c" (result[2]), "=d" (result[3])
    :  "0" (eax), "c" (ecx)
    );
}

//
// TSC.
//

inline uint64_t ia32_asm_read_tsc() noexcept
{
  uint32_t eax, edx;

  asm volatile(
    "rdtsc"
    : "=a" (eax), "=d" (edx)
    );

  return uint64_t(eax) | uint64_t(edx) << 32;
}

inline uint64_t ia32_asm_read_tscp(uint32_t* aux) noexcept
{
  uint32_t eax, edx;

  asm volatile(
    "rdtscp"
    : "=c" (*aux), "=a" (eax), "=d" (edx)
    );

  return uint64_t(eax) | uint64_t(edx) << 32;
}

//
// Float state save/restore.
//

inline void ia32_asm_fx_save(void* fxarea) noexcept
{
  asm volatile(
    "fxsaveq %0"
    : "=m" (fxarea)
    );
}

inline void ia32_asm_fx_restore(const void* fxarea) noexcept
{
  asm volatile(
    "fxrstorq %0"
    :
    : "m" (fxarea)
    );
}

//
// Pause/halt.
//

inline void ia32_asm_pause() noexcept
{
  asm volatile(
    "pause"
    );
}

inline void ia32_asm_halt() noexcept
{
  asm volatile(
    "hlt"
    );
}

//
// Segment registers.
//

inline uint16_t ia32_asm_read_cs() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%cs, %0"
    : "=r" (result)
    );

  return result;
}

inline void ia32_asm_write_cs(uint16_t cs) noexcept
{
  asm volatile(
    "movw %0, %%cs"
    :
    : "r" (cs)
    );
}

inline uint16_t ia32_asm_read_ds() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%ds, %0"
    : "=r" (result)
    );

  return result;
}

inline void ia32_asm_write_ds(uint16_t ds) noexcept
{
  asm volatile(
    "movw %0, %%ds"
    :
    : "r" (ds)
    );
}

inline uint16_t ia32_asm_read_es() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%es, %0"
    : "=r" (result)
    );

  return result;
}

inline void ia32_asm_write_es(uint16_t es) noexcept
{
  asm volatile(
    "movw %0, %%es"
    :
    : "r" (es)
    );
}

inline uint16_t ia32_asm_read_fs() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%fs, %0"
    : "=r" (result)
    );

  return result;
}

inline void ia32_asm_write_fs(uint16_t fs) noexcept
{
  asm volatile(
    "movw %0, %%fs"
    :
    : "r" (fs)
    );
}

inline uint16_t ia32_asm_read_gs() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%gs, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_gs(uint16_t gs) noexcept
{
  asm volatile(
    "movw %0, %%gs"
    :
    : "r" (gs)
    );
}

inline uint16_t ia32_asm_read_ss() noexcept
{
  uint16_t result;

  asm volatile(
    "movw %%ss, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_ss(uint16_t ss) noexcept
{
  asm volatile(
    "movw %0, %%ss"
    :
    : "r" (ss)
    );
}

inline uint16_t ia32_asm_read_tr() noexcept
{
  uint16_t result;

  asm volatile(
    "str %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_tr(uint16_t tr) noexcept
{
  asm volatile(
    "ltr %0"
    :
    : "r" (tr)
    );
}

inline uint16_t ia32_asm_read_ldtr() noexcept
{
  uint16_t result;

  asm volatile(
    "sldt %0"
    :  "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_ldtr(uint16_t ldt) noexcept
{
  asm volatile(
    "lldt %0"
    :
    : "r" (ldt)
    );
}

inline uint32_t ia32_asm_read_ar(uint16_t selector) noexcept
{
  uint32_t result;

  asm volatile(
    "lar %1, %0"
    : "=r" (result)
    : "r" (selector)
    );
  
  return result;
}

inline uint32_t ia32_asm_read_sl(uint32_t segment) noexcept
{
  uint32_t result;

  asm volatile(
    "lsl %1, %0"
    : "=r" (result)
    : "r" (segment)
    );
  
  return result;
}

//
// Descriptor registers.
//

inline void ia32_asm_read_gdtr(void* gdt) noexcept
{
  asm volatile(
    "sgdt %0"
    : "=m" (*(uint8_t*)gdt)
    );
}

inline void ia32_asm_write_gdtr(const void* gdt) noexcept
{
  asm volatile(
    "lgdt %0"
    :
    : "m" (*(uint8_t*)gdt)
    );
}

inline void ia32_asm_read_idtr(void* idt) noexcept
{
  asm volatile(
    "sidt %0"
    : "=m" (*(uint8_t*)idt)
    );
}

inline void ia32_asm_write_idtr(void* idt) noexcept
{
  asm volatile(
    "lidt %0"
    :
    : "m" (*(uint8_t*)idt)
    );
}

//
// Interrupts.
//

inline void ia32_asm_enable_interrupts() noexcept
{
  asm volatile(
    "sti"
    :
    :
    : "memory"
    );
}

inline void ia32_asm_disable_interrupts() noexcept
{
  asm volatile(
    "cli"
    :
    :
    : "memory"
    );
}

//
// I/O ports.
//

inline uint8_t ia32_asm_in_byte(uint16_t port) noexcept
{
  uint8_t result;

  asm volatile(
    "in %1, %0"
    : "=a" (result)
    : "Nd" (port)
    );

  return result;
}

inline uint16_t ia32_asm_in_word(uint16_t port) noexcept
{
  uint16_t result;

  asm volatile(
    "in %1, %0"
    : "=a" (result)
    : "Nd" (port)
    );

  return result;
}

inline uint32_t ia32_asm_in_dword(uint16_t port) noexcept
{
  uint32_t result;

  asm volatile(
    "in %1, %0"
    : "=a" (result)
    : "Nd" (port)
    );

  return result;
}

inline void ia32_asm_in_byte_string(uint16_t port, uint8_t* data, uint32_t size) noexcept
{
  asm volatile(
    "rep ; insb"
    : "+D" (data), "+c" (size)
    : "d" (port)
    );
}

inline void ia32_asm_in_word_string(uint16_t port, uint16_t* data, uint32_t size) noexcept
{
  asm volatile(
    "rep ; insw"
    : "+D" (data), "+c" (size)
    : "d" (port)
    );
}

inline void ia32_asm_in_dword_string(uint16_t port, uint32_t* data, uint32_t size) noexcept
{
  asm volatile(
    "rep ; insl"
    : "+D" (data), "+c" (size)
    : "d" (port)
    );
}

inline void ia32_asm_out_byte(uint16_t port, uint8_t value) noexcept
{
  asm volatile(
    "out %0, %1"
    :
    : "a" (value), "Nd" (port)
    );
}

inline void ia32_asm_out_word(uint16_t port, uint16_t value) noexcept
{
  asm volatile(
    "out %0, %1"
    :
    : "a" (value), "Nd" (port)
    );
}

inline void ia32_asm_out_dword(uint16_t port, uint32_t value) noexcept
{
  asm volatile(
    "out %0, %1"
    :
    : "a" (value), "Nd" (port)
    );
}

inline void ia32_asm_out_byte_string(uint16_t port, uint8_t* data, uint32_t count) noexcept
{
  asm volatile(
    "rep ; outsb"
    : "+S" (data), "+c" (count)
    : "d" (port)
    );
}

inline void ia32_asm_out_word_string(uint16_t port, uint16_t* data, uint32_t count) noexcept
{
  asm volatile(
    "rep ; outsw"
    : "+S" (data), "+c" (count)
    : "d" (port)
    );
}

inline void ia32_asm_out_dword_string(uint16_t port, uint32_t* data, uint32_t count) noexcept
{
  asm volatile(
    "rep ; outsl"
    : "+S" (data), "+c" (count)
    : "d" (port)
    );
}

//
// Control registers.
//

inline void ia32_asm_clear_ts(void) noexcept
{
  asm volatile(
    "clts"
    );
}

inline void ia32_asm_write_msw(uint16_t msw) noexcept
{
  asm volatile(
    "lmsw %0"
    :
    : "r" (msw)
    );
}

inline uint64_t ia32_asm_read_cr0() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%cr0, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_cr0(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%cr0"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_cr2() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%cr2, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_cr2(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%cr2"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_cr3() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%cr3, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_cr3(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%cr3"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_cr4() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%cr4, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_cr4(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%cr4"
    :
    : "r" (value)
    );
}

//
// Debug registers.
//

inline uint64_t ia32_asm_read_dr0() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr0, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr0(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr0"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr1() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr1, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr1(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr1"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr2() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr2, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr2(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr2"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr3() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr3, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr3(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr3"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr4() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr4, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr4(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr4"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr5() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr5, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr5(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr5"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr6() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr6, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr6(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr6"
    :
    : "r" (value)
    );
}

inline uint64_t ia32_asm_read_dr7() noexcept
{
  uint64_t result;

  asm volatile(
    "movq %%dr7, %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_dr7(uint64_t value) noexcept
{
  asm volatile(
    "movq %0, %%dr7"
    :
    : "r" (value)
    );
}

//
// EFLAGS/RFLAGS.
//

inline uint64_t ia32_asm_read_eflags() noexcept
{
  uint64_t result;

  asm volatile(
    "pushfq; popq %0"
    : "=r" (result)
    );
  
  return result;
}

inline void ia32_asm_write_eflags(uint64_t value) noexcept
{
  asm volatile(
    "pushq %0; popfq"
    :
    : "r" (value)
    );
}

//
// MSRs.
//


inline uint64_t ia32_asm_read_msr(uint32_t msr) noexcept
{
  uint32_t eax, edx;

  asm volatile(
    "rdmsr"
    : "=a" (eax), "=d" (edx) : "c" (msr)
    );

  return uint64_t(eax) | uint64_t(edx) << 32;
}

inline void ia32_asm_write_msr(uint32_t msr, uint64_t value) noexcept
{
  uint32_t eax = uint32_t(value);
  uint32_t edx = uint32_t(value >> 32);

  asm volatile(
    "wrmsr"
    :
    : "a" (eax), "d" (edx), "c" (msr)
    );
}

//
// XCRs.
//

inline uint64_t ia32_asm_read_xcr(uint32_t reg) noexcept
{
  uint32_t eax, edx;

  asm volatile(
    "xgetbv"
    : "=a" (eax), "=d" (edx) : "c" (reg)
    );

  return uint64_t(eax) | uint64_t(edx) << 32;
}

inline void ia32_asm_write_xcr(uint32_t reg, uint64_t value) noexcept
{
  uint32_t eax = uint32_t(value);
  uint32_t edx = uint32_t(value >> 32);

  asm volatile(
    "xsetbv"
    :
    : "c" (reg), "a" (eax), "d" (edx)
    );
}

//
// Bit operations.
//

inline uint32_t ia32_asm_bsf(uint64_t value) noexcept
{
  uint64_t result;

  asm volatile(
    "bsfq %1, %0"
    : "=r" (result)
    : "rm" (value)
    );

  return uint32_t(result);
}

inline uint32_t ia32_asm_bsr(uint64_t value) noexcept
{
  uint64_t result;

  asm volatile(
    "bsrq %1, %0"
    : "=r" (result)
    : "rm" (value)
    );

  return uint32_t(result);
}

inline uint8_t ia32_asm_bt(const void* base, uint64_t offset) noexcept
{
  uint64_t result;

  asm volatile(
    "btq %2, %1\n\t"
    "setc %b0\n\t"
    "andq $1, %0\n\t"
    : "=q" (result)
    : "m" (*(const volatile long*)base),
      "Ir" (offset)
    : "memory"
    );

  return uint8_t(result);
}

inline uint8_t ia32_asm_bts(void* base, uint64_t offset) noexcept
{
  uint64_t result;

  asm volatile(
    "btsq %2, %1\n\t"
    "setc %b0\n\t"
    "andq $1, %0\n\t"
    : "=q" (result),
      "=m" (*(volatile long*)base)
    : "Ir" (offset),
      "m" (*(volatile long*)base)
    : "memory"
    );

  return uint8_t(result);
}

inline uint64_t ia32_asm_popcnt(uint64_t value) noexcept
{
  uint64_t result;
  
  asm volatile(
    "popcntq %1, %0"
    : "=r" (result)
    : "rm" (value)
    );
  
  return result;
}

//
// Cache control.
//

inline void ia32_asm_invd() noexcept
{
  asm volatile(
    "invd"
    );
}

inline void ia32_asm_wb_invd(void) noexcept
{
  asm volatile(
    "wbinvd"
    );
}

inline void ia32_asm_inv_page(void* address) noexcept
{
  asm volatile(
    "invlpg %0"
    :
    : "m" (*(char *)address) : "memory"
    );
}

inline void ia32_asm_inv_pcid(invpcid_t type, invpcid_desc_t* descriptor) noexcept
{
  asm volatile(
    "invpcid"
    :
    : "d" (descriptor), "a" ((uint64_t)type)
    : "memory"
    );
}

//
// VMX.
//

#define ASM_VMX_VMCLEAR_RAX       ".byte 0x66, 0x0f, 0xc7, 0x30"
#define ASM_VMX_VMLAUNCH          ".byte 0x0f, 0x01, 0xc2"
#define ASM_VMX_VMRESUME          ".byte 0x0f, 0x01, 0xc3"
#define ASM_VMX_VMPTRLD_RAX       ".byte 0x0f, 0xc7, 0x30"
#define ASM_VMX_VMXOFF            ".byte 0x0f, 0x01, 0xc4"
#define ASM_VMX_VMXON_RAX         ".byte 0xf3, 0x0f, 0xc7, 0x30"
#define ASM_VMX_INVEPT            ".byte 0x66, 0x0f, 0x38, 0x80, 0x0A"
#define ASM_VMX_INVVPID           ".byte 0x66, 0x0f, 0x38, 0x81, 0x0A"
#define ASM_VMX_VMFUNC            ".byte 0x0f, 0x01, 0xd4"

inline uint8_t ia32_asm_vmx_on(uint64_t* vmxon_pa) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_VMXON_RAX "; setna %0"
    : "=q" (error)
    : "a" (vmxon_pa), "m" (*vmxon_pa)
    : "memory", "cc"
    );

  return error;
}

inline void ia32_asm_vmx_off(void) noexcept
{
  asm volatile(
    "vmxoff"
    );
}

inline uint8_t ia32_asm_vmx_vmlaunch(void) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_VMLAUNCH "; setna %0"
    : "=q" (error)
    : /* no reads  */
    : "cc"
    );

  return error;
}

inline uint8_t ia32_asm_vmx_vmresume(void) noexcept
{
  uint8_t error = 0;

  asm volatile(
    "vmresume"
    );

  return error;
}

inline uint8_t ia32_asm_vmx_vmclear(uint64_t* vmcs_pa) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_VMCLEAR_RAX "; setna %0"
    : "=qm" (error)
    : "a" (vmcs_pa), "m" (*vmcs_pa)
    : "cc", "memory"
    );

  return error;
}

inline uint8_t ia32_asm_vmx_vmread(uint64_t vmcs_field, uint64_t* value) noexcept
{
  uint8_t error = 0;

  asm volatile(
    "vmread %2, %0; setna %1"
    : "=r" (*value), "=qm" (error)
    : "r" (vmcs_field)
    : "cc"
    );

  return error;
}

inline uint8_t ia32_asm_vmx_vmwrite(uint64_t vmcs_field, uint64_t value) noexcept
{
  uint8_t error = 0;

  asm volatile(
    "vmwrite %1, %2; setna %0"
    : "=qm" (error)
    : "r" (value), "r" (vmcs_field)
    : "cc"
    );

  return error;
}

inline uint64_t ia32_asm_vmx_vmcall(uint64_t rcx, uint64_t rdx, uint64_t r8, uint64_t r9) noexcept
{
  asm volatile(
    "vmcall"
    :
    : "c" (rcx), "d" (rdx)
    : "cc"
    );
  
  return 0;
}

inline void ia32_asm_vmx_vmptr_read(uint64_t* vmcs_pa) noexcept
{
  asm volatile(
    "vmptrst %0"
    : "=m" (*vmcs_pa)
    :
    : "memory"
    );
}

inline uint8_t ia32_asm_vmx_vmptr_write(uint64_t* vmcs_pa) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_VMPTRLD_RAX "; setna %0"
    : "=qm" (error)
    : "a" (vmcs_pa), "m" (*vmcs_pa)
    : "cc", "memory"
    );

  return error;
}

inline uint8_t ia32_asm_inv_ept(invept_t type, invept_desc_t* descriptor) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_INVEPT "; setna %0"
    : "=q" (error)
    : "d" (descriptor), "c" (type)
    : "cc", "memory"
    );
  
  return error;
}

inline uint8_t ia32_asm_inv_vpid(invvpid_t type, invvpid_desc_t* descriptor) noexcept
{
  uint8_t error = 0;

  asm volatile(
    ASM_VMX_INVVPID "; setna %0"
    : "=q" (error)
    : "d" (descriptor), "c" (type)
    : "cc", "memory"
    );

  return error;
}

#ifdef __cplusplus
}
#endif

//
// This macro expands to code which will cause compiler to print error message
// which includes size of the object.
//
#define static_sizeof(object)                 \
  do                                          \
  {                                           \
    switch (*reinterpret_cast<int*>(nullptr)) \
    {                                         \
      case sizeof(object): break;             \
      case sizeof(object): break;             \
    }                                         \
  } while (0)
