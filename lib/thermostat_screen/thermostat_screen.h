#pragma once

#include <stdint.h>

/// What the thermostat screen shows. Plain data, filled in by the project.
struct ThermostatView {
  enum class Status { kWaiting, kOk, kAlarm, kSilenced, kSensorError };

  bool has_reading = false;  // false until the first good sensor read
  float temp_f = 0.0f;
  float humidity_pct = 0.0f;
  int threshold_f = 0;
  Status status = Status::kWaiting;
};

/// The kit's 160x80 ST7735 TFT (AX22-0034) on the shared SPI bus, showing a
/// ThermostatView. Draws into an off-screen canvas and pushes it in one
/// transfer, which avoids flicker (catalog requirement). The caller decides
/// when to redraw: only when the view changed, at most ~30 fps.
class ThermostatScreen {
 public:
  /// @param cs, rst, dc  the port's IO pins (pins::kTftCs/kTftRst/kTftDc).
  ThermostatScreen(uint8_t cs, uint8_t rst, uint8_t dc);

  /// Start SPI and initialise the panel. Returns false if the frame buffer
  /// could not be allocated or the SPI bus failed to start. (The panel itself
  /// gives no acknowledgement over SPI, so a disconnected screen can only be
  /// spotted by eye.)
  bool begin();

  /// Render the view and push it to the panel (~25 KB over SPI).
  void draw(const ThermostatView& view);

 private:
  uint8_t cs_, rst_, dc_;
  bool ready_ = false;
};
