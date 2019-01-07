#pragma once
#include "vmexit.h"

#include "lib/typelist.h"

#include <tuple>

namespace hvpp
{
  template <
    typename ...ARGS
  >
  class vmexit_compositor_handler
    : public vmexit_handler
  {
    public:
      using vmexit_handler_tuple_t = std::tuple<ARGS...>;
      vmexit_handler_tuple_t handlers;

      auto initialize() noexcept -> error_code_t override
      {
        error_code_t err;

        //
        // Initialize all handlers.
        // If initialization of one or more handlers fail, error
        // code of only the first failed initialization is saved
        // and returned.  Initialization of other handlers doesn't
        // stop on the first error.
        //
        for_each_element(handlers, [&](auto&& handler, int) {
          auto local_err = handler.initialize();

          if (!err)
          {
            err = local_err;
          }
        });

        return err;
      }

      void destroy() noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.destroy();
        });
      }

      void setup(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.setup(vp);
        });
      }

      void handle(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.handle(vp);
        });
      }

      void invoke_termination(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.invoke_termination(vp);
        });
      }
  };

}
