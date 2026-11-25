#include "conversions.h"

namespace conversions {

float celsiusToFahrenheit(float celsius) { return celsius * 9.0f / 5.0f + 32.0f; }

uint16_t panelColor(uint8_t r, uint8_t g, uint8_t b) {
  // Standard RGB565 packing, fed blue where red goes and red where blue goes.
  const uint16_t hi = static_cast<uint16_t>(b & 0xF8) << 8;
  const uint16_t mid = static_cast<uint16_t>(g & 0xFC) << 3;
  const uint16_t lo = static_cast<uint16_t>(r >> 3);
  return static_cast<uint16_t>(hi | mid | lo);
}

}  // namespace conversions
