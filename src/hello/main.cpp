// hello: the first program to flash on a new Genesis Mini.
//
// Prints a boot report (chip, flash, PSRAM, reset reason), scans the I2C bus,
// then blinks a heartbeat on the on-board NeoPixel and logs uptime every 10 s.
// Green heartbeat = all checks passed; red heartbeat = see the serial log.

#include <Arduino.h>

#include "board_info.h"
#include "i2c_bus.h"
#include "periodic.h"
#include "pins.h"
#include "status_led.h"

namespace {

constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kSerialWaitMs = 2000;   // give the USB host time to attach
constexpr uint32_t kUptimeLogMs = 10000;
constexpr StatusLed::Color kOk = {0, 12, 4};
constexpr StatusLed::Color kFault = {16, 0, 0};

StatusLed g_led(pins::kNeoPixel, kOk);
Periodic g_uptimeLog(kUptimeLogMs);

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  // Bounded wait (setup only) so the boot report isn't lost before the serial
  // monitor connects; runs fine with no computer attached.
  const uint32_t start = millis();
  while (!Serial && millis() - start < kSerialWaitMs) {
    delay(10);
  }

  Serial.println("\n[hello] Genesis Mini starting");
  bool healthy = board_info::printBootReport(Serial);
  if (i2c_bus::begin(Serial)) {
    healthy = (i2c_bus::scan(Serial) >= 0) && healthy;
  } else {
    healthy = false;
  }

  g_led.setColor(healthy ? kOk : kFault);
  g_led.begin();
  g_uptimeLog.reset(millis());
  Serial.printf("[hello] %s\n", healthy ? "all checks passed" : "checks FAILED, see above");
}

void loop() {
  const uint32_t now = millis();
  g_led.update(now);
  if (g_uptimeLog.due(now)) {
    Serial.printf("[hello] uptime %" PRIu32 " s, free heap %" PRIu32 " B\n", now / 1000u,
                  ESP.getFreeHeap());
  }
}
