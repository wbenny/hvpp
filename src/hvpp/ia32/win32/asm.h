#pragma once
#include <intrin.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

//
// ref: Vol2A[(INVPCID—Invalidate Process-Context Identifier)]
//

enum class invpcid_t : uint32_t
{
  individual_address                = 0x00000000,
  single_context                    = 0x00000001,
  all_contexts                      = 0x00000002,
  all_contexts_retaining_globals    = 0x00000003,
};

struct invpcid_desc_t
{
  uint64_t pcid : 12;
  uint64_t reserved : 52;
  uint64_t linear_address;
};

static_assert(sizeof(invpcid_desc_t) == 16);

//
// These instructions aren't provided by the intrin.h header.
//

unsigned short      ia32_asm_read_cs            ()                            noexcept;
void                ia32_asm_write_cs           (_In_ unsigned short cs)      noexcept;
unsigned short      ia32_asm_read_ds            ()                            noexcept;
void                ia32_asm_write_ds           (_In_ unsigned short ds)      noexcept;
unsigned short      ia32_asm_read_es            ()                            noexcept;
void                ia32_asm_write_es           (_In_ unsigned short es)      noexcept;
unsigned short      ia32_asm_read_fs            ()                            noexcept;
void                ia32_asm_write_fs           (_In_ unsigned short fs)      noexcept;
unsigned short      ia32_asm_read_gs            ()                            noexcept;
void                ia32_asm_write_gs           (_In_ unsigned short gs)      noexcept;
unsigned short      ia32_asm_read_ss            ()                            noexcept;
void                ia32_asm_write_ss           (_In_ unsigned short ss)      noexcept;

unsigned short      ia32_asm_read_tr            ()                            noexcept;
void                ia32_asm_write_tr           (_In_ unsigned short tr)      noexcept;

unsigned short      ia32_asm_read_ldtr          ()                            noexcept;
void                ia32_asm_write_ldtr         (_In_ unsigned short ldt)     noexcept;

unsigned long       ia32_asm_read_ar            (_In_ unsigned short selector)noexcept;

//
// This breaks release build. MSVC __segmentlimit in combination
// with being in the "if constexpr" block generates invalid code.
//
// lsl instruction is replaced with custom asm implementation.
//
//#define           ia32_asm_read_sl            __segmentlimit
unsigned long       ia32_asm_read_sl            (_In_ unsigned long seg)      noexcept;

void                ia32_asm_read_gdtr          (_Out_ void* gdt)             noexcept;
void                ia32_asm_write_gdtr         (_In_ void* gdt)              noexcept;

void                ia32_asm_invd               ()                            noexcept;
void                ia32_asm_halt               ()                            noexcept;
void                ia32_asm_write_msw          (_In_ unsigned short msw)     noexcept;
unsigned long long  ia32_asm_vmx_vmcall         (_In_ unsigned long long rcx, _In_ unsigned long long rdx, _In_ unsigned long long r8, _In_ unsigned long long r9) noexcept;
unsigned char       ia32_asm_inv_ept            (_In_ unsigned long type, _In_ void* descriptor) noexcept;
unsigned char       ia32_asm_inv_vpid           (_In_ unsigned long type, _In_ void* descriptor) noexcept;

//
// MSVC intrinsics.
//

#define             ia32_asm_int3               __debugbreak

#define             ia32_asm_cpuid              __cpuid
#define             ia32_asm_cpuid_ex           __cpuidex

#define             ia32_asm_fx_save            _fxsave
#define             ia32_asm_fx_restore         _fxrstor

#define             ia32_asm_pause              _mm_pause

#define             ia32_asm_enable_interrupts  _enable
#define             ia32_asm_disable_interrupts _disable

#define             ia32_asm_read_tsc           __rdtsc
#define             ia32_asm_read_tscp          __rdtscp

#define             ia32_asm_in_byte            __inbyte
#define             ia32_asm_in_word            __inword
#define             ia32_asm_in_dword           __indword
#define             ia32_asm_in_byte_string     __inbytestring
#define             ia32_asm_in_word_string     __inwordstring
#define             ia32_asm_in_dword_string    __indwordstring
#define             ia32_asm_out_byte           __outbyte
#define             ia32_asm_out_word           __outword
#define             ia32_asm_out_dword          __outdword
#define             ia32_asm_out_byte_string    __outbytestring
#define             ia32_asm_out_word_string    __outwordstring
#define             ia32_asm_out_dword_string   __outdwordstring

#define             ia32_asm_read_idtr          __sidt
#define             ia32_asm_write_idtr         __lidt

#define             ia32_asm_read_cr0           __readcr0
#define             ia32_asm_write_cr0          __writecr0
#define             ia32_asm_read_cr2           __readcr2
#define             ia32_asm_write_cr2          __writecr2
#define             ia32_asm_read_cr3           __readcr3
#define             ia32_asm_write_cr3          __writecr3
#define             ia32_asm_read_cr4           __readcr4
#define             ia32_asm_write_cr4          __writecr4
#define             ia32_asm_read_dr            __readdr
#define             ia32_asm_write_dr           __writedr
#define             ia32_asm_read_eflags        __readeflags
#define             ia32_asm_write_eflags       __writeeflags
#define             ia32_asm_read_msr           __readmsr
#define             ia32_asm_write_msr          __writemsr

#define             ia32_asm_read_xcr           _xgetbv
#define             ia32_asm_write_xcr          _xsetbv

#define             ia32_asm_popcnt             __popcnt64
#define             ia32_asm_clear_ts           __clts
#define             ia32_asm_wb_invd            __wbinvd
#define             ia32_asm_inv_page           __invlpg

#define             ia32_asm_vmx_on             __vmx_on
#define             ia32_asm_vmx_off            __vmx_off
#define             ia32_asm_vmx_vmlaunch       __vmx_vmlaunch
#define             ia32_asm_vmx_vmresume       __vmx_vmresume
#define             ia32_asm_vmx_vmclear        __vmx_vmclear
#define             ia32_asm_vmx_vmread         __vmx_vmread
#define             ia32_asm_vmx_vmwrite        __vmx_vmwrite
#define             ia32_asm_vmx_vmptr_read     __vmx_vmptrst
#define             ia32_asm_vmx_vmptr_write    __vmx_vmptrld

inline
unsigned long       ia32_asm_bsf(_In_ unsigned long long word) noexcept
{
  unsigned long result;
  _BitScanForward64(&result, word);
  return result;
}

inline
unsigned long      ia32_asm_bsr(_In_ unsigned long long word) noexcept
{
  unsigned long result;
  _BitScanReverse64(&result, word);
  return result;
}

inline
unsigned char      ia32_asm_bt(_In_ const void* base, _In_ unsigned long offset) noexcept
{
  return _bittest((const long*)base, offset);
}

inline
unsigned char      ia32_asm_bts(_In_ void* base, _In_ unsigned long offset) noexcept
{
  return _bittestandset((long*)base, offset);
}

inline
void               ia32_asm_inv_pcid(_In_ invpcid_t type, _In_opt_ invpcid_desc_t* descriptor = nullptr) noexcept
{
  if (!descriptor)
  {
    static invpcid_desc_t zero_descriptor{};
    descriptor = &zero_descriptor;
  }

  _invpcid(static_cast<unsigned int>(type), descriptor);
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
