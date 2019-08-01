#pragma once
#include "hvpp/vmexit.h"

#include "hvpp/lib/bitmap.h"

#include <atomic>

namespace hvpp {

//
// Structure for storing statistics about VM-exits.
// Each member of the vmexit_storage_t structure
// is treated as VM-exit counter.
//
using vmexit_stats_storage_t = vmexit_storage_t<uint32_t>;

//
// Simple VM-exit handler which performs statistics about VM-exits
// and also allows their tracing (by hvpp_trace()).
//

class vmexit_stats_handler
  : public vmexit_handler
{
  public:
    vmexit_stats_handler() noexcept;
    ~vmexit_stats_handler() noexcept override;

    void handle(vcpu_t& vp) noexcept override;

    bitmap<>& trace_bitmap() noexcept
    { return vmexit_trace_bitmap_; }

    vmexit_stats_storage_t* storage() const noexcept
    { return storage_ ; }

    void dump() noexcept;

  private:
    //
    // Update "lhs" stats by adding to them values of "rhs" stats.
    //
    void storage_merge(vmexit_stats_storage_t& lhs, const vmexit_stats_storage_t& rhs) const noexcept;

    //
    // Dump this stats structure.
    //
    void storage_dump(const vmexit_stats_storage_t& storage_to_dump) const noexcept;

    //
    // Array of statistics (per VCPU).
    //
    vmexit_stats_storage_t* storage_;

    //
    // Merged statistics.
    // Used in dump() method.
    //
    vmexit_stats_storage_t storage_merged_;

    //
    // Bitmap of traced VM-exit reasons.
    // There are currently defined 65 VM-exit reasons.
    //
    bitmap<65> vmexit_trace_bitmap_;

    //
    // Count of terminated VCPUs.
    //
    std::atomic<uint32_t> terminated_vcpu_count_;
};

}
