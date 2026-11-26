#include "led_button.h"

#include <Arduino.h>

LedButton::LedButton(uint8_t button_pin, uint8_t led_pin) : button_pin_(button_pin), led_pin_(led_pin) {}

void LedButton::begin() {
  pinMode(button_pin_, INPUT);  // the module has its own pull-up
  pinMode(led_pin_, OUTPUT);
  digitalWrite(led_pin_, LOW);
  led_on_ = false;
  was_down_ = digitalRead(button_pin_) == LOW;
}

bool LedButton::pressed() {
  const bool down = digitalRead(button_pin_) == LOW;
  const bool edge = down && !was_down_;
  was_down_ = down;
  return edge;
}

void LedButton::setLed(bool on) {
  if (on != led_on_) {
    digitalWrite(led_pin_, on ? HIGH : LOW);
    led_on_ = on;
  }
}
