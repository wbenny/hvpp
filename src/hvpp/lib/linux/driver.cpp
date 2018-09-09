#include "../driver.h"
#include "../device.h"

#include "lib/error.h"

extern "C" {

void driver_unload(void)
{
  driver::common::destroy();
}

void driver_entry(void)
{
  driver::common::initialize();
}

}
