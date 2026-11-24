#pragma once

#include <stdint.h>

class Print;

/// The board's shared I2C bus (AX22 ports + STEMMA QT connector). SDA/SCL
/// come from the board variant (GPIO10/GPIO11), so no pins are passed here.
namespace i2c_bus {

/// Start the bus with the board's default pins. Logs and returns false on
/// failure. Safe to call more than once.
bool begin(Print& log);

/// Probe every 7-bit address and print the ones that answer. Returns the
/// number of devices found, or -1 if the bus reported an error.
int scan(Print& log);

}  // namespace i2c_bus
