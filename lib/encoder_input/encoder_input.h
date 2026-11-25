#pragma once

#include <stdint.h>

/// The kit's rotary encoder with push button (AX22-0003): turns detents and
/// presses into events. poll() must be called every loop(); the encoder is
/// sampled there, so keep loop() free of long work.
class EncoderInput {
 public:
  struct Event {
    int32_t steps;    // detents turned since the last poll (+ clockwise)
    bool pressed;     // one debounced press, reported the moment it happens
    bool long_press;  // reported once when the knob has been held kLongPressMs
  };

  /// How long the knob must be held for a long press.
  static constexpr uint32_t kLongPressMs = 1500;

  /// @param button, clk, dt the port's IO0/IO1/IO2 (pins::kEncoder*).
  EncoderInput(uint8_t button, uint8_t clk, uint8_t dt);

  /// Configure the pins. Call once from setup().
  void begin();

  Event poll(uint32_t now_ms);

 private:
  uint8_t button_, clk_, dt_;
  int32_t last_position_ = 0;
};
