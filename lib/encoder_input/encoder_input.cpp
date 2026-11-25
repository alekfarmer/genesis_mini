#include "encoder_input.h"

#include <Arduino.h>
#include <RotaryEncoder.h>

#include "debounce.h"

namespace {

// Catalog rules for AX22-0003: LatchMode::TWO03 (one step per detent), button
// active HIGH with the module's own pull-down (plain INPUT), 150 ms lockout.
constexpr uint32_t kButtonLockoutMs = 150;

// Sign that makes clockwise positive. -1 was confirmed on the kit's AX22-0003
// (the library counts clockwise as negative with these CLK/DT pins).
constexpr int32_t kDirection = -1;

RotaryEncoder* g_encoder = nullptr;
Debounce g_button(true, kButtonLockoutMs);

}  // namespace

EncoderInput::EncoderInput(uint8_t button, uint8_t clk, uint8_t dt) : button_(button), clk_(clk), dt_(dt) {}

void EncoderInput::begin() {
  static RotaryEncoder encoder(clk_, dt_, RotaryEncoder::LatchMode::TWO03);
  g_encoder = &encoder;
  pinMode(button_, INPUT);
  last_position_ = static_cast<int32_t>(encoder.getPosition());
}

EncoderInput::Event EncoderInput::poll(uint32_t now_ms) {
  Event e{0, false};
  if (g_encoder == nullptr) {
    return e;
  }
  g_encoder->tick();
  const int32_t position = static_cast<int32_t>(g_encoder->getPosition());
  e.steps = (position - last_position_) * kDirection;
  last_position_ = position;
  e.pressed = g_button.pressed(digitalRead(button_) == HIGH, now_ms);
  return e;
}
