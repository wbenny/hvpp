#pragma once
#include <ntddk.h>

#define hvpp_assert(expression) do { if (!(expression)) { __debugbreak(); } } while (0)
