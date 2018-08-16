#pragma once
#include <windows.h>

BOOL
ForEachLogicalCore(
  void (*CallbackFunction)(void*),
  void* Context
  );
