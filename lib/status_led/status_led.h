#pragma once

#include <stdint.h>

/// Drives the on-board WS2812 NeoPixel as a status light with the heartbeat
/// pattern. Uses the Arduino-ESP32 core's built-in rgbLedWrite() (RMT-based),
/// so no NeoPixel library is needed.
class StatusLed {
 public:
  struct Color {
    uint8_t r, g, b;
  };

  /// @param pin   data GPIO of the NeoPixel (pins::kNeoPixel).
  /// @param color colour while lit; keep it dim, the LED is very bright.
  explicit StatusLed(uint8_t pin, Color color = {0, 12, 4});

  /// Turn the LED off and start the pattern. Call once from setup().
  void begin();

  /// Change the colour, e.g. to signal an error state.
  void setColor(Color color);

  /// Advance the pattern. Call every loop(); only writes to the LED when the
  /// on/off state changes, so it is cheap to call often.
  void update(uint32_t now_ms);

 private:
  void write(bool on);

  uint8_t pin_;
  Color color_;
  bool lit_ = false;
};
