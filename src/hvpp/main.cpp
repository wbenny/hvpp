#include "lib/driver.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "lib/mm.h"
#include "lib/mp.h"

#include "hvpp/hypervisor.h"

#include "custom_vmexit.h"

#include <cinttypes>

namespace driver
{
  using vmexit_handler_t = custom_vmexit_handler;
  static_assert(std::is_base_of_v<hvpp::vmexit_handler, vmexit_handler_t>);

  hvpp::hypervisor*     hypervisor;
  hvpp::vmexit_handler* vmexit_handler;

  auto initialize() noexcept -> error_code_t
  {
    //
    // Create hypervisor instance.
    //
    hypervisor = new hvpp::hypervisor();

    if (!hypervisor)
    {
      destroy();
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Initialize hypervisor.
    //
    if (auto err = hypervisor->initialize())
    {
      destroy();
      return err;
    }

    //
    // Create VM-exit handler instance.
    //
    vmexit_handler = new vmexit_handler_t();
    if (!vmexit_handler)
    {
      destroy();
      return make_error_code_t(std::errc::not_enough_memory);
    }

    //
    // Initialize VM-exit handler.
    //
    if (auto err = vmexit_handler->initialize())
    {
      destroy();
      return err;
    }

    //
    // Start the hypervisor.
    //
    hypervisor->start(vmexit_handler);

    //
    // Tell debugger we're started.
    //
    hvpp_info("Hypervisor started, current free memory: %" PRIu64 " MB",
               memory_manager::free_bytes() / 1024 / 1024);

    return error_code_t{};
  }

  void destroy() noexcept
  {
    //
    // Stop and destroy hypervisor.
    //
    if (hypervisor)
    {
      hypervisor->stop();
      hypervisor->destroy();
      delete hypervisor;
    }

    //
    // Destroy VM-exit handler.
    //
    if (vmexit_handler)
    {
      vmexit_handler->destroy();
      delete vmexit_handler;
    }

    hvpp_info("Hypervisor stopped");
  }
}
