// hello: the first program to flash on a new Genesis Mini.
//
// Prints a boot report (chip, flash, PSRAM, reset reason), scans the I2C bus,
// then blinks a heartbeat on the on-board NeoPixel and logs uptime every 10 s.
// Green heartbeat = all checks passed; red heartbeat = see the serial log.
//
// The report is printed again whenever a serial monitor connects: on the
// ESP32-S3's native USB, a reset drops the USB connection, and output printed
// before the monitor reattaches is lost.

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
bool g_i2cUp = false;
bool g_hostWasConnected = false;

/// Print the board report and scan I2C. Returns true if every check passed.
/// Run at boot and again each time a serial monitor connects.
bool runChecks() {
  bool healthy = board_info::printBootReport(Serial);
  if (g_i2cUp) {
    healthy = (i2c_bus::scan(Serial) >= 0) && healthy;
  } else {
    Serial.println("[i2c] ERROR: bus not started, see boot log");
    healthy = false;
  }
  Serial.printf("[hello] %s\n", healthy ? "all checks passed" : "checks FAILED, see above");
  return healthy;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  // Bounded wait (setup only) so a monitor that is already attached sees the
  // boot report; runs fine with no computer attached.
  const uint32_t start = millis();
  while (!Serial && millis() - start < kSerialWaitMs) {
    delay(10);
  }

  Serial.println("\n[hello] Genesis Mini starting");
  g_i2cUp = i2c_bus::begin(Serial);
  const bool healthy = runChecks();

  g_led.setColor(healthy ? kOk : kFault);
  g_led.begin();
  g_uptimeLog.reset(millis());
  g_hostWasConnected = static_cast<bool>(Serial);
}

void loop() {
  const uint32_t now = millis();
  g_led.update(now);

  // A monitor just attached: repeat the report it missed.
  const bool hostConnected = static_cast<bool>(Serial);
  if (hostConnected && !g_hostWasConnected) {
    Serial.println("\n[hello] serial monitor connected, repeating checks");
    g_led.setColor(runChecks() ? kOk : kFault);
  }
  g_hostWasConnected = hostConnected;

  if (g_uptimeLog.due(now)) {
    Serial.printf("[hello] uptime %" PRIu32 " s, free heap %" PRIu32 " B\n", now / 1000u,
                  ESP.getFreeHeap());
  }
}
