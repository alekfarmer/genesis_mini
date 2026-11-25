#pragma once

#include <stddef.h>
#include <stdint.h>

#include "panel.h"
#include "status_led.h"
#include "tone_sequence.h"

/// The arcade's app framework. Each game (and the menu) is an App; the host
/// (src/arcade/main.cpp) owns the hardware, turns knob and button activity
/// into Input, and calls the active app's update() and draw().
///
/// To add a game: implement App in lib/<game>_app, then add one line to the
/// list in src/arcade/apps.cpp.
namespace arcade {

/// Everything the player did since the previous update().
struct Input {
  int32_t knob_steps;      // detents turned (+ = clockwise)
  bool knob_pressed;       // the moment the knob goes down
  bool button_tap;         // on-board button: a short press (on release)
  bool button_hold_start;  // on-board button: held past ~220 ms
  bool button_held;        // ...and still held
  bool button_hold_end;    // ...released after a hold
};

class App {
 public:
  virtual ~App() = default;

  /// Shown in the menu; keep it to 10 characters or fewer.
  virtual const char* name() const = 0;

  /// Preferences namespace for this app's best score, or nullptr if it has
  /// none. The menu shows the saved best next to the name.
  virtual const char* scoreKey() const { return nullptr; }

  /// Screen orientation while this app runs. The host switches the panel.
  virtual panel::Orientation orientation() const { return panel::Orientation::kLandscape; }

  /// Called when the app is opened from the menu: start fresh.
  virtual void enter(uint32_t now_ms) = 0;

  /// Called every loop() while active. Must not block.
  virtual void update(const Input& in, uint32_t now_ms) = 0;

  /// True if something visible changed since the last draw().
  virtual bool needsRedraw() const = 0;

  /// Draw the whole screen into panel::canvas(). The host pushes it to the
  /// panel afterwards and limits redraws to ~30 per second.
  virtual void draw() = 0;

  /// Called when leaving the app (back to the menu).
  virtual void exit() {}
};

// ---- Services the host provides to apps -------------------------------------

/// Play a one-shot sound, replacing whatever was playing. `steps` must stay
/// valid while it plays (use static arrays).
void playSound(const ToneSequence::Step* steps, size_t count, uint32_t now_ms);

/// Silence the buzzer.
void stopSound();

/// Set the NeoPixel heartbeat colour.
void setLed(StatusLed::Color color);

/// Best score saved under `key` (0 if none). Errors are logged to serial.
uint32_t loadBest(const char* key);

/// Save a new best score under `key`. Errors are logged to serial.
void saveBest(const char* key, uint32_t score);

/// A fresh random seed from the hardware random number generator.
uint32_t newSeed();

/// Common LED colours.
constexpr StatusLed::Color kLedNormal = {0, 12, 4};
constexpr StatusLed::Color kLedFlash = {24, 24, 24};
constexpr StatusLed::Color kLedBad = {16, 0, 0};

/// Presses within this long after a game ends are ignored, so the press that
/// was meant for the game doesn't immediately restart it.
constexpr uint32_t kRestartGuardMs = 600;

}  // namespace arcade
