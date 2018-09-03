#include "lib/debugger.h"
#include "lib/driver.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "lib/mm.h"
#include "lib/mp.h"

#include "hvpp/hypervisor.h"

#include "vmexit_custom.h"
#include "hvpp/vmexit_compositor.h"

#include <cinttypes>

namespace driver
{
  //
  // Create combined handler from these VM-exit handlers.
  //
  using vmexit_handler_t = vmexit_compositor_handler<
    vmexit_stats_handler,
    vmexit_dbgbreak_handler,
    vmexit_custom_handler
    >;

  static_assert(std::is_base_of_v<hvpp::vmexit_handler, vmexit_handler_t>);

  hvpp::hypervisor* hypervisor;
  vmexit_handler_t* vmexit_handler;

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
    // Enable tracing of I/O instructions.
    //
    std::get<hvpp::vmexit_stats_handler>(vmexit_handler->handlers)
      .trace_bitmap().set(int(ia32::vmx::exit_reason::execute_io_instruction));

    //
    // If debugger is enabled, break on first IN 0x64
    // and OUT 0x64 instruction.
    //
    if (debugger::is_enabled())
    {
      std::get<hvpp::vmexit_dbgbreak_handler>(vmexit_handler->handlers)
        .storage().io_in[0x64] = true;

      std::get<hvpp::vmexit_dbgbreak_handler>(vmexit_handler->handlers)
        .storage().io_out[0x64] = true;
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
      //
      // Print statistics into debugger.
      //
      std::get<vmexit_stats_handler>(vmexit_handler->handlers).dump();

      vmexit_handler->destroy();
      delete vmexit_handler;
    }

    hvpp_info("Hypervisor stopped");
  }
}
