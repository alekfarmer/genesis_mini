#include "status_led.h"

#include <Arduino.h>

#include "heartbeat.h"

StatusLed::StatusLed(uint8_t pin, Color color) : pin_(pin), color_(color) {}

void StatusLed::begin() {
  lit_ = false;
  write(false);
}

void StatusLed::setColor(Color color) {
  color_ = color;
  if (lit_) {
    write(true);
  }
}

void StatusLed::update(uint32_t now_ms) {
  const bool on = heartbeat::isOn(now_ms);
  if (on != lit_) {
    lit_ = on;
    write(on);
  }
}

void StatusLed::write(bool on) {
  // rgbLedWrite() returns void in core 3.3.x, so there is no status to check;
  // a dead LED shows up as "no heartbeat" in the manual check (README).
  if (on) {
    rgbLedWrite(pin_, color_.r, color_.g, color_.b);
  } else {
    rgbLedWrite(pin_, 0, 0, 0);
  }
}
