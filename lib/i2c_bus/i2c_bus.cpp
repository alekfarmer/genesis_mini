#include "i2c_bus.h"

#include <Arduino.h>
#include <Wire.h>

namespace i2c_bus {
namespace {

// Wire.endTransmission() result codes (Arduino-ESP32).
constexpr uint8_t kAck = 0;
constexpr uint8_t kNackOnAddress = 2;

// Valid 7-bit device addresses (0x00-0x07 and 0x78-0x7F are reserved).
constexpr uint8_t kFirstAddress = 0x08;
constexpr uint8_t kLastAddress = 0x77;

}  // namespace

bool begin(Print& log) {
  if (!Wire.begin()) {
    log.printf("[i2c] ERROR: Wire.begin() failed on SDA=%u SCL=%u\n",
               static_cast<unsigned>(SDA), static_cast<unsigned>(SCL));
    return false;
  }
  log.printf("[i2c] bus up on SDA=%u SCL=%u\n", static_cast<unsigned>(SDA),
             static_cast<unsigned>(SCL));
  return true;
}

int scan(Print& log) {
  int found = 0;
  for (uint8_t addr = kFirstAddress; addr <= kLastAddress; ++addr) {
    Wire.beginTransmission(addr);
    const uint8_t result = Wire.endTransmission();
    if (result == kAck) {
      log.printf("[i2c] device at 0x%02X\n", addr);
      ++found;
    } else if (result != kNackOnAddress) {
      log.printf("[i2c] ERROR: bus error %u at 0x%02X, scan aborted\n",
                 static_cast<unsigned>(result), addr);
      return -1;
    }
  }
  log.printf("[i2c] scan done, %d device(s)\n", found);
  return found;
}

}  // namespace i2c_bus
