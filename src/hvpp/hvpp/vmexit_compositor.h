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

      vmexit_compositor_handler() noexcept
      { }

      ~vmexit_compositor_handler() noexcept override
      { }

      void setup(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.setup(vp);
        });
      }

      void teardown(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.teardown(vp);
        });
      }

      void handle(vcpu_t& vp) noexcept override
      {
        for_each_element(handlers, [&](auto&& handler, int) {
          handler.handle(vp);
        });
      }
  };

}
