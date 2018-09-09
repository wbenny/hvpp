#include "../cr3_guard.h"

namespace detail {

ia32::cr3_t kernel_cr3(ia32::cr3_t cr3) noexcept
{
  return cr3;
}

}
