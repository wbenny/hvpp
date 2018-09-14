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
  __debugbreak();
}

//
// CPUID.
//

void __cpuid(int[4], int);
#pragma intrinsic(__cpuid)
inline void ia32_asm_cpuid(uint32_t result[4], uint32_t eax) noexcept
{
  __cpuid((int*)result, (int)eax);
}

void __cpuidex(int[4], int, int);
#pragma intrinsic(__cpuidex)
inline void ia32_asm_cpuid_ex(uint32_t result[4], uint32_t eax, uint32_t ecx) noexcept
{
  __cpuidex((int*)result, (int)eax, (int)ecx);
}

//
// TSC.
//

unsigned __int64 __rdtsc(void);
#pragma intrinsic(__rdtsc)
inline uint64_t ia32_asm_read_tsc() noexcept
{
  return __rdtsc();
}

unsigned __int64 __rdtscp(unsigned int*);
#pragma intrinsic(__rdtscp)
inline uint64_t ia32_asm_read_tscp(uint32_t* aux) noexcept
{
  return __rdtscp(aux);
}

//
// Float state save/restore.
//

void _fxsave(void*);
#pragma intrinsic(_fxsave)
inline void ia32_asm_fx_save(void* fxarea) noexcept
{
  _fxsave(fxarea);
}

void _fxrstor(void const*);
#pragma intrinsic(_fxrstor)
inline void ia32_asm_fx_restore(const void* fxarea) noexcept
{
  _fxrstor(fxarea);
}

//
// Pause/halt.
//

void _mm_pause(void);
#pragma intrinsic(_mm_pause)
inline void ia32_asm_pause() noexcept
{
  _mm_pause();
}

void ia32_asm_halt() noexcept;

//
// Segment registers.
//

uint16_t ia32_asm_read_cs() noexcept;
void ia32_asm_write_cs(uint16_t cs) noexcept;
uint16_t ia32_asm_read_ds() noexcept;
void ia32_asm_write_ds(uint16_t ds) noexcept;
uint16_t ia32_asm_read_es() noexcept;
void ia32_asm_write_es(uint16_t es) noexcept;
uint16_t ia32_asm_read_fs() noexcept;
void ia32_asm_write_fs(uint16_t fs) noexcept;
uint16_t ia32_asm_read_gs() noexcept;
void ia32_asm_write_gs(uint16_t gs) noexcept;
uint16_t ia32_asm_read_ss() noexcept;
void ia32_asm_write_ss(uint16_t ss) noexcept;
uint16_t ia32_asm_read_tr() noexcept;
void ia32_asm_write_tr(uint16_t tr) noexcept;
uint16_t ia32_asm_read_ldtr() noexcept;
void ia32_asm_write_ldtr(uint16_t ldt) noexcept;

uint32_t ia32_asm_read_ar(uint16_t selector) noexcept;
uint32_t ia32_asm_read_sl(uint32_t segment) noexcept;

//
// Descriptor registers.
//

void ia32_asm_read_gdtr(void* gdt) noexcept;
void ia32_asm_write_gdtr(const void* gdt) noexcept;

void __sidt(void*);
#pragma intrinsic(__sidt)
inline void ia32_asm_read_idtr(void* idt) noexcept
{
  __sidt(idt);
}

void __lidt(void*);
#pragma intrinsic(__lidt)
inline void ia32_asm_write_idtr(void* idt) noexcept
{
  __lidt(idt);
}

//
// Interrupts.
//

void _enable(void);
#pragma intrinsic(_enable)
inline void ia32_asm_enable_interrupts() noexcept
{
  _enable();
}

void _disable(void);
#pragma intrinsic(_disable)
inline void ia32_asm_disable_interrupts() noexcept
{
  _disable();
}

//
// I/O ports.
//

unsigned char __inbyte(unsigned short);
#pragma intrinsic(__inbyte)
inline uint8_t ia32_asm_in_byte(uint16_t port) noexcept
{
  return __inbyte(port);
}

unsigned short __inword(unsigned short);
#pragma intrinsic(__inword)
inline uint16_t ia32_asm_in_word(uint16_t port) noexcept
{
  return __inword(port);
}

unsigned long __indword(unsigned short);
#pragma intrinsic(__indword)
inline uint32_t ia32_asm_in_dword(uint16_t port) noexcept
{
  return __indword(port);
}

void __inbytestring(unsigned short, unsigned char*, unsigned long);
#pragma intrinsic(__inbytestring)
inline void ia32_asm_in_byte_string(uint16_t port, uint8_t* data, uint32_t size) noexcept
{
  __inbytestring(port, data, size);
}

void __inwordstring(unsigned short, unsigned short*, unsigned long);
#pragma intrinsic(__inwordstring)
inline void ia32_asm_in_word_string(uint16_t port, uint16_t* data, uint32_t size) noexcept
{
  __inwordstring(port, data, size);

}

void __indwordstring(unsigned short, unsigned long*, unsigned long);
#pragma intrinsic(__indwordstring)
inline void ia32_asm_in_dword_string(uint16_t port, uint32_t* data, uint32_t size) noexcept
{
  __indwordstring(port, (unsigned long*)data, size);

}

void __outbyte(unsigned short, unsigned char);
#pragma intrinsic(__outbyte)
inline void ia32_asm_out_byte(uint16_t port, uint8_t value) noexcept
{
  __outbyte(port, value);
}

void __outword(unsigned short, unsigned short);
#pragma intrinsic(__outword)
inline void ia32_asm_out_word(uint16_t port, uint16_t value) noexcept
{
  __outword(port, value);
}

void __outdword(unsigned short, unsigned long);
#pragma intrinsic(__outdword)
inline void ia32_asm_out_dword(uint16_t port, uint32_t value) noexcept
{
  __outdword(port, value);
}

void __outbytestring(unsigned short, unsigned char*, unsigned long);
#pragma intrinsic(__outbytestring)
inline void ia32_asm_out_byte_string(uint16_t port, uint8_t* data, uint32_t count) noexcept
{
  __outbytestring(port, data, count);
}

void __outwordstring(unsigned short, unsigned short*, unsigned long);
#pragma intrinsic(__outwordstring)
inline void ia32_asm_out_word_string(uint16_t port, uint16_t* data, uint32_t count) noexcept
{
  __outwordstring(port, data, count);
}

void __outdwordstring(unsigned short, unsigned long*, unsigned long);
#pragma intrinsic(__outdwordstring)
inline void ia32_asm_out_dword_string(uint16_t port, uint32_t* data, uint32_t count) noexcept
{
  __outdwordstring(port, (unsigned long*)data, count);
}

//
// Control registers.
//

void __clts(void);
#pragma intrinsic(__clts)
inline void ia32_asm_clear_ts(void) noexcept
{
  __clts();
}

void ia32_asm_write_msw(uint16_t msw) noexcept;

unsigned __int64 __readcr0(void);
#pragma intrinsic(__readcr0)
inline uint64_t ia32_asm_read_cr0() noexcept
{
  return __readcr0();
}

void __writecr0(unsigned __int64);
#pragma intrinsic(__writecr0)
inline void ia32_asm_write_cr0(uint64_t value) noexcept
{
  __writecr0(value);
}

unsigned __int64 __readcr2(void);
#pragma intrinsic(__readcr2)
inline uint64_t ia32_asm_read_cr2() noexcept
{
  return __readcr2();
}

void __writecr2(unsigned __int64);
#pragma intrinsic(__writecr2)
inline void ia32_asm_write_cr2(uint64_t value) noexcept
{
  __writecr2(value);
}

unsigned __int64 __readcr3(void);
#pragma intrinsic(__readcr3)
inline uint64_t ia32_asm_read_cr3() noexcept
{
  return __readcr3();
}

void __writecr3(unsigned __int64);
#pragma intrinsic(__writecr3)
inline void ia32_asm_write_cr3(uint64_t value) noexcept
{
  __writecr3(value);
}

unsigned __int64 __readcr4(void);
#pragma intrinsic(__readcr4)
inline uint64_t ia32_asm_read_cr4() noexcept
{
  return __readcr4();
}

void __writecr4(unsigned __int64);
#pragma intrinsic(__writecr4)
inline void ia32_asm_write_cr4(uint64_t value) noexcept
{
  __writecr4(value);
}

//
// Debug registers.
//

unsigned __int64 __readdr(unsigned int);
#pragma intrinsic(__readdr)
inline uint64_t ia32_asm_read_dr0() noexcept
{
  return __readdr(0);
}

void __writedr(unsigned int, unsigned __int64);
#pragma intrinsic(__writedr)
inline void ia32_asm_write_dr0(uint64_t value) noexcept
{
  return __writedr(0, value);
}

inline uint64_t ia32_asm_read_dr1() noexcept
{
  return __readdr(1);
}

inline void ia32_asm_write_dr1(uint64_t value) noexcept
{
  return __writedr(1, value);
}

inline uint64_t ia32_asm_read_dr2() noexcept
{
  return __readdr(2);
}

inline void ia32_asm_write_dr2(uint64_t value) noexcept
{
  return __writedr(2, value);
}

inline uint64_t ia32_asm_read_dr3() noexcept
{
  return __readdr(3);
}

inline void ia32_asm_write_dr3(uint64_t value) noexcept
{
  return __writedr(3, value);
}

inline uint64_t ia32_asm_read_dr4() noexcept
{
  return __readdr(4);
}

inline void ia32_asm_write_dr4(uint64_t value) noexcept
{
  return __writedr(4, value);
}

inline uint64_t ia32_asm_read_dr5() noexcept
{
  return __readdr(5);
}

inline void ia32_asm_write_dr5(uint64_t value) noexcept
{
  return __writedr(5, value);
}

inline uint64_t ia32_asm_read_dr6() noexcept
{
  return __readdr(6);
}

inline void ia32_asm_write_dr6(uint64_t value) noexcept
{
  return __writedr(6, value);
}

inline uint64_t ia32_asm_read_dr7() noexcept
{
  return __readdr(7);
}

inline void ia32_asm_write_dr7(uint64_t value) noexcept
{
  return __writedr(7, value);

}

//
// EFLAGS/RFLAGS.
//

unsigned __int64 __readeflags(void);
#pragma intrinsic(__readeflags)
inline uint64_t ia32_asm_read_eflags() noexcept
{
  return __readeflags();
}

void __writeeflags(unsigned __int64);
#pragma intrinsic(__writeeflags)
inline void ia32_asm_write_eflags(uint64_t value) noexcept
{
  __writeeflags(value);
}

//
// MSRs.
//

unsigned __int64 __readmsr(unsigned long);
#pragma intrinsic(__readmsr)
inline uint64_t ia32_asm_read_msr(uint32_t msr) noexcept
{
  return __readmsr(msr);
}

void __writemsr(unsigned long, unsigned __int64);
#pragma intrinsic(__writemsr)
inline void ia32_asm_write_msr(uint32_t msr, uint64_t value) noexcept
{
  __writemsr(msr, value);
}

//
// XCRs.
//

unsigned __int64 _xgetbv(unsigned int);
#pragma intrinsic(_xgetbv)
inline uint64_t ia32_asm_read_xcr(uint32_t reg) noexcept
{
  return _xgetbv(reg);
}

void _xsetbv(unsigned int, unsigned __int64);
#pragma intrinsic(_xsetbv)
inline void ia32_asm_write_xcr(uint32_t reg, uint64_t value) noexcept
{
  _xsetbv(reg, value);
}

//
// Bit operations.
//

unsigned char _BitScanForward64(unsigned long*, unsigned __int64);
#pragma intrinsic(_BitScanForward64)
inline uint32_t ia32_asm_bsf(uint64_t value) noexcept
{
  uint32_t result;
  _BitScanForward64((unsigned long*)&result, value);
  return result;
}

unsigned char _BitScanReverse64(unsigned long*, unsigned __int64);
#pragma intrinsic(_BitScanReverse64)
inline uint32_t ia32_asm_bsr(uint64_t value) noexcept
{
  uint32_t result;
  _BitScanReverse64((unsigned long*)&result, value);
  return result;
}

unsigned char _bittest64(__int64 const*, __int64);
#pragma intrinsic(_bittest64)
inline uint8_t ia32_asm_bt(const void* base, uint64_t offset) noexcept
{
  return _bittest64((const __int64*)base, offset);
}

unsigned char _bittestandset64(__int64*, __int64);
#pragma intrinsic(_bittestandset64)
inline uint8_t ia32_asm_bts(void* base, uint64_t offset) noexcept
{
  return _bittestandset64((__int64*)base, offset);
}

unsigned __int64 __popcnt64(unsigned __int64);
#pragma intrinsic(__popcnt64)
inline uint64_t ia32_asm_popcnt(uint64_t value) noexcept
{
  return __popcnt64(value);
}

//
// Cache control.
//

void ia32_asm_invd() noexcept;

void __wbinvd(void);
#pragma intrinsic(__wbinvd)
inline void ia32_asm_wb_invd(void) noexcept
{
  __wbinvd();
}

void __invlpg(void*);
#pragma intrinsic(__invlpg)
inline void ia32_asm_inv_page(void* address) noexcept
{
  __invlpg(address);
}

void _invpcid(unsigned int, void*);
#pragma intrinsic(_invpcid)
inline void ia32_asm_inv_pcid(invpcid_t type, invpcid_desc_t* descriptor) noexcept
{
  _invpcid((unsigned int)type, descriptor);
}

//
// VMX.
//

unsigned char __vmx_on(unsigned __int64*);
#pragma intrinsic(__vmx_on)
inline uint8_t ia32_asm_vmx_on(uint64_t* vmxon_pa) noexcept
{
  return __vmx_on(vmxon_pa);
}


void __vmx_off(void);
#pragma intrinsic(__vmx_off)
inline void ia32_asm_vmx_off(void) noexcept
{
  __vmx_off();
}

unsigned char __vmx_vmlaunch(void);
#pragma intrinsic(__vmx_vmlaunch)
inline uint8_t ia32_asm_vmx_vmlaunch(void) noexcept
{
  return __vmx_vmlaunch();
}

unsigned char __vmx_vmresume(void);
#pragma intrinsic(__vmx_vmresume)
inline uint8_t ia32_asm_vmx_vmresume(void) noexcept
{
  return __vmx_vmresume();
}

unsigned char __vmx_vmclear(unsigned __int64*);
#pragma intrinsic(__vmx_vmclear)
inline uint8_t ia32_asm_vmx_vmclear(uint64_t* vmcs_pa) noexcept
{
  return __vmx_vmclear(vmcs_pa);
}

unsigned char __vmx_vmread(size_t, size_t*);
#pragma intrinsic(__vmx_vmread)
inline uint8_t ia32_asm_vmx_vmread(uint64_t vmcs_field, uint64_t* value) noexcept
{
  return __vmx_vmread(vmcs_field, value);
}

unsigned char __vmx_vmwrite(size_t, size_t);
#pragma intrinsic(__vmx_vmwrite)
inline uint8_t ia32_asm_vmx_vmwrite(uint64_t vmcs_field, uint64_t value) noexcept
{
  return __vmx_vmwrite(vmcs_field, value);
}

uint64_t ia32_asm_vmx_vmcall(uint64_t rcx, uint64_t rdx, uint64_t r8, uint64_t r9) noexcept;


void __vmx_vmptrst(unsigned __int64*);
#pragma intrinsic(__vmx_vmptrst)
inline void ia32_asm_vmx_vmptr_read(uint64_t* vmcs_pa) noexcept
{
  __vmx_vmptrst(vmcs_pa);
}

unsigned char __vmx_vmptrld(unsigned __int64*);
#pragma intrinsic(__vmx_vmptrld)
inline uint8_t ia32_asm_vmx_vmptr_write(uint64_t* vmcs_pa) noexcept
{
  return __vmx_vmptrld(vmcs_pa);
}

uint8_t ia32_asm_inv_ept(invept_t type, invept_desc_t* descriptor) noexcept;
uint8_t ia32_asm_inv_vpid(invvpid_t type, invvpid_desc_t* descriptor) noexcept;

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
