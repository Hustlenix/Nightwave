#pragma once
#include "driver/i2c_master.h"
namespace nightwave {
// Call once in app_main before starting tasks. No bus is made in bench mode.
bool prepare_board_i2c();
// Serialized creation/retry; the bus lives for the entire program. Devices borrow it.
i2c_master_bus_handle_t reference_i2c_bus();
}
