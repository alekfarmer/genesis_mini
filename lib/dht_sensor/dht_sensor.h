#pragma once

#include <stdint.h>

/// The kit's DHT11 temperature & humidity module (AX22-0011). The DHT11 can
/// be read at most once per second; the caller schedules reads (kit_demo:
/// every 2 s).
class DhtSensor {
 public:
  struct Reading {
    bool ok;             // false if either value failed to read
    float temp_c;
    float humidity_pct;
  };

  /// @param pin the port's IO1 alias (pins::kDht).
  explicit DhtSensor(uint8_t pin);

  /// Start the sensor. Call once from setup().
  void begin();

  /// Read temperature and humidity once. Takes about 25 ms (the DHT11's
  /// single-wire protocol is timed by the library with interrupts off).
  Reading read();

 private:
  uint8_t pin_;
};
