// probe: an experiment to see which AX22 modules can be detected.
//
// Every 3 s (and whenever a serial monitor connects) it reads each port pin
// twice, once with the chip's weak internal pull-up on and once with its
// pull-down on, and prints what it sees next to the module board_config.h
// says is there. A module with its own pull resistor holds a pin at its
// level either way; an empty port follows the internal pull. It also scans
// the shared I2C bus.
//
// Safety: pins are only ever read (input with a weak ~45 kOhm pull), never
// driven, so nothing attached can be harmed. Side effects are harmless: the
// screen may reset (its RST pin is read) and the LED button's LED may glow
// faintly while its pin is pulled up.
//
// Run it with modules plugged in and unplugged and compare the tables:
// pins whose state changes reliably are the ones detection can use.

#include <Arduino.h>

#include "i2c_bus.h"
#include "periodic.h"
#include "pin_probe.h"
#include "pins.h"
#include "status_led.h"

namespace {

constexpr uint32_t kReportMs = 3000;
constexpr uint32_t kSettleUs = 500;  // let the weak pull charge the pin and module

StatusLed g_led(pins::kNeoPixel, {0, 6, 12});
Periodic g_report(kReportMs);
bool g_i2cUp = false;
bool g_hostWasConnected = false;

const char* moduleName(int m) {
  switch (m) {
    case MOD_NONE:       return "empty";
    case MOD_TFT:        return "screen AX22-0034";
    case MOD_BUZZER:     return "buzzer AX22-0018";
    case MOD_DHT11:      return "DHT11 AX22-0011";
    case MOD_ENCODER:    return "encoder AX22-0003";
    case MOD_LED_BUTTON: return "LED button AX22-0050";
    default:             return "?";
  }
}

/// Read one pin with each internal pull, then leave it as a plain input.
pin_probe::PinState probePin(uint8_t gpio) {
  pinMode(gpio, INPUT_PULLUP);
  delayMicroseconds(kSettleUs);  // brief settle; the probe does nothing else
  const bool high_up = digitalRead(gpio) == HIGH;
  pinMode(gpio, INPUT_PULLDOWN);
  delayMicroseconds(kSettleUs);
  const bool high_down = digitalRead(gpio) == HIGH;
  pinMode(gpio, INPUT);
  return pin_probe::classify(high_up, high_down);
}

void report() {
  Serial.println("\n[probe] port  configured            IO0          IO1          IO2");
  for (int port = 1; port <= 4; ++port) {
    Serial.printf("[probe] P%d    %-21s", port, moduleName(pins::portModule(port)));
    for (int io = 0; io < 3; ++io) {
      const uint8_t gpio = pins::portPin(port, io);
      Serial.printf(" %-12s", pin_probe::describe(probePin(gpio)));
    }
    Serial.println();
  }
  if (g_i2cUp) {
    i2c_bus::scan(Serial);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  const uint32_t start = millis();
  while (!Serial && millis() - start < 2000) {
    delay(10);  // bounded wait for a monitor; setup only
  }
  Serial.println("\n[probe] starting: reading port pins with pull-up / pull-down (read-only)");
  g_i2cUp = i2c_bus::begin(Serial);
  g_led.begin();
  report();
  g_report.reset(millis());
}

void loop() {
  const uint32_t now = millis();
  const bool host = static_cast<bool>(Serial);
  if ((host && !g_hostWasConnected) || g_report.due(now)) {
    report();
  }
  g_hostWasConnected = host;
  g_led.update(now);
}
