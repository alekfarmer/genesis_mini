#pragma once

#include <stdint.h>

/// Turns a raw, bouncy button level into single "pressed" events.
///
/// A press is reported on the edge from released to pressed, and only if at
/// least lockout_ms has passed since the previous press, which swallows contact
/// bounce. Holding the button reports one press. Pure logic, host-tested.
class Debounce {
 public:
  /// @param active_high true if the pressed level is HIGH (e.g. the AX22
  ///                    encoder button), false for active-low buttons.
  /// @param lockout_ms  minimum time between two reported presses.
  Debounce(bool active_high, uint32_t lockout_ms);

  /// Feed the current raw level; returns true exactly once per press.
  bool pressed(bool raw_level, uint32_t now_ms);

 private:
  bool active_high_;
  uint32_t lockout_ms_;
  bool was_pressed_ = false;
  bool has_pressed_before_ = false;
  uint32_t last_press_ms_ = 0;
};
