#pragma once

#include <stdint.h>

/// Small unit and colour conversions. Pure logic, host-tested.
namespace conversions {

/// Degrees Celsius to degrees Fahrenheit.
float celsiusToFahrenheit(float celsius);

/// RGB888 colour to the 16-bit value for the kit's ST7735 panel. The panel
/// is BGR, so red and blue are swapped before packing into RGB565 (see the
/// AX22-0034 catalog notes). Use this instead of the ST77XX_* colour constants.
uint16_t panelColor(uint8_t r, uint8_t g, uint8_t b);

}  // namespace conversions
