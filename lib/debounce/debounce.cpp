#include "debounce.h"

Debounce::Debounce(bool active_high, uint32_t lockout_ms)
    : active_high_(active_high), lockout_ms_(lockout_ms) {}

bool Debounce::pressed(bool raw_level, uint32_t now_ms) {
  const bool is_pressed = (raw_level == active_high_);
  const bool rising_edge = is_pressed && !was_pressed_;
  was_pressed_ = is_pressed;
  if (!rising_edge) {
    return false;
  }
  // Unsigned subtraction keeps the lockout correct across a millis() wrap.
  if (has_pressed_before_ && (now_ms - last_press_ms_) < lockout_ms_) {
    return false;
  }
  has_pressed_before_ = true;
  last_press_ms_ = now_ms;
  return true;
}
