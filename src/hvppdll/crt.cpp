#include "crt.h"

// +++++++++++++++++++++++++++++++++++++++++++
//
// Prototypes.
//
// -------------------------------------------

//
// Put .CRT data into .rdata section
//
#pragma comment(linker, "/merge:.CRT=.rdata")

//
// C initializer function prototype.
//
typedef int (__cdecl *_PIFV)(void);

//
// Linker puts constructors between these sections, and we use them to locate constructor pointers.
//
#pragma section(".CRT$XIA", long, read)
#pragma section(".CRT$XIZ", long, read)

//
// Pointers surrounding constructors.
//
__declspec(allocate(".CRT$XIA")) _PIFV __xi_a[] = { 0 };
__declspec(allocate(".CRT$XIZ")) _PIFV __xi_z[] = { 0 };
extern __declspec(allocate(".CRT$XIA")) _PIFV __xi_a[];
extern __declspec(allocate(".CRT$XIZ")) _PIFV __xi_z[];

static int __cdecl
_initterm_e(
  _PIFV* const first,
  _PIFV* const last
  );

//
// C++ initializer function prototype.
//
typedef void (__cdecl *_PVFV)(void);

//
// Linker puts constructors between these sections, and we use them to locate constructor pointers.
//
#pragma section(".CRT$XCA", long, read)
#pragma section(".CRT$XCZ", long, read)

//
// Pointers surrounding constructors.
//
__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { 0 };
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { 0 };
extern __declspec(allocate(".CRT$XCA")) _PVFV __xc_a[];
extern __declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[];

static void __cdecl
_initterm(
  _PVFV* const first,
  _PVFV* const last
  );

// +++++++++++++++++++++++++++++++++++++++++++
//
// Implemetation.
//
// -------------------------------------------

int __cdecl
_initterm_e(
  _PIFV* const first,
  _PIFV* const last
  )
{
  //
  // C initialization.
  //
  for (_PIFV* it = first; it != last; ++it)
  {
    if (*it == nullptr)
    {
      continue;
    }

    const int result = (**it)();
    if (result != 0)
    {
      return result;
    }
  }

  return 0;
}

void __cdecl
_initterm(
  _PVFV* const first,
  _PVFV* const last
  )
{
  //
  // C++ initialization.
  //
  for (_PVFV* it = first; it != last; ++it)
  {
    if (*it == nullptr)
      continue;

    (**it)();
  }
}

VOID
NTAPI
CrtInitialize(
  VOID
  )
{
  //
  // Call C initializers.
  //
  _initterm_e(__xi_a, __xi_z);

  //
  // Call C++ initializers.
  //
  _initterm(__xc_a, __xc_z);
}

VOID
NTAPI
CrtDestroy(
  VOID
  )
{

}
