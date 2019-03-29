#pragma once
#include "hvpp/vmexit.h"

#include <atomic>

namespace hvpp {

//
// Structure for storing on which VM-exits the debug-break
// should be invoked.
//
using vmexit_dbgbreak_storage_t = vmexit_storage_t<std::atomic_bool>;

//
// Simple handler which breaks into the debugger
// on specified VM-exit.  Each breakpoint is hit
// only once.
//

class vmexit_dbgbreak_handler
  : public vmexit_handler
{
  public:
    vmexit_dbgbreak_handler() noexcept;
    ~vmexit_dbgbreak_handler() noexcept override;

    void handle(vcpu_t& vp) noexcept override;

    vmexit_dbgbreak_storage_t& storage() noexcept
    { return storage_; }

  private:
    vmexit_dbgbreak_storage_t storage_;
};

}
