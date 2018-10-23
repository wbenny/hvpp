// #include <hvpp/hypervisor.h>
// 
// #include <hvpp/lib/driver.h>
// #include <hvpp/lib/assert.h>
// #include <hvpp/lib/log.h>
// #include <hvpp/lib/mm.h>
// #include <hvpp/lib/mp.h>
// 
// #include "hvpp_vmexit_handler.h"
// #include "hvpp_device.h"
// 
// #include <cinttypes>
// 
// using namespace ia32;
// using namespace hvpp;
// 
// namespace driver
// {
//   hvpp_device*    device_          = nullptr;
// 
//   auto initialize() noexcept -> error_code_t
//   {
//     //
//     // Create device instance.
//     //
//     device_ = new hvpp_device();
// 
//     if (!device_)
//     {
//       destroy();
//       return make_error_code_t(std::errc::not_enough_memory);
//     }
// 
//     //
//     // Initialize device instance.
//     //
//     if (auto err = device_->initialize())
//     {
//       destroy();
//       return err;
//     }
// 
//     return error_code_t{};
//   }
// 
//   void destroy() noexcept
//   {
//     //
//     // Destroy device.
//     //
//     if (device_)
//     {
//       device_->destroy();
//       delete device_;
//     }
//   }
// }
