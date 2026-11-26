#pragma once

#include <stdint.h>

/// The Tactile LED Button module (AX22-0050): a push button and an
/// independently controlled white LED. The module debounces the button in
/// hardware (RC filter), so presses are taken from the HIGH -> LOW edge with
/// no software debounce (catalog rule).
class LedButton {
 public:
  /// @param button_pin the port's IO1 (pins::kLedButton), LOW when pressed.
  /// @param led_pin    the port's IO2 (pins::kLedButtonLed), HIGH = on.
  LedButton(uint8_t button_pin, uint8_t led_pin);

  /// Configure the pins and turn the LED off. Call once from setup().
  void begin();

  /// True once per press (the moment it goes down). Call every loop().
  bool pressed();

  /// Turn the button's LED on or off.
  void setLed(bool on);

 private:
  uint8_t button_pin_;
  uint8_t led_pin_;
  bool was_down_ = false;
  bool led_on_ = false;
};
