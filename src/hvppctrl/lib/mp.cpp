#include "mp.h"

BOOL
ForEachLogicalCore(
  void (*CallbackFunction)(void*),
  void* Context
  )
{
  GROUP_AFFINITY OriginalGroupAffinity;
  if (!GetThreadGroupAffinity(GetCurrentThread(), &OriginalGroupAffinity))
  {
    return FALSE;
  }

  BOOL Result = FALSE;
  WORD GroupCount = GetActiveProcessorGroupCount();
  for (WORD GroupNumber = 0; GroupNumber < GroupCount; ++GroupNumber)
  {
    DWORD ProcessorCount = GetActiveProcessorCount(GroupNumber);

    for (DWORD ProcessorNumber = 0; ProcessorNumber < ProcessorCount; ++ProcessorNumber)
    {
      GROUP_AFFINITY GroupAffinity = { 0 };
      GroupAffinity.Mask = (KAFFINITY)(1) << ProcessorNumber;
      GroupAffinity.Group = GroupNumber;
      if (!SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, NULL))
      {
        goto exit;
      }

      CallbackFunction(Context);
    }
  }

  Result = TRUE;

exit:
  SetThreadGroupAffinity(GetCurrentThread(), &OriginalGroupAffinity, NULL);
  return Result;
}
