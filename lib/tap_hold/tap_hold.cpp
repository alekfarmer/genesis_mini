#include "tap_hold.h"

TapHold::TapHold(bool active_high, uint32_t hold_ms, uint32_t settle_ms)
    : active_high_(active_high), hold_ms_(hold_ms), settle_ms_(settle_ms) {}

TapHold::Event TapHold::update(bool raw_level, uint32_t now_ms) {
  const bool raw_pressed = (raw_level == active_high_);

  // Track how long the raw level has been unchanged; accept it once stable.
  if (raw_pressed != candidate_pressed_) {
    candidate_pressed_ = raw_pressed;
    candidate_since_ms_ = now_ms;
  }
  if (candidate_pressed_ != stable_pressed_ && now_ms - candidate_since_ms_ >= settle_ms_) {
    stable_pressed_ = candidate_pressed_;
    if (stable_pressed_) {
      press_start_ms_ = candidate_since_ms_;
      holding_ = false;
    } else if (holding_) {
      holding_ = false;
      return Event::kHoldEnd;
    } else {
      return Event::kTap;
    }
  }

  if (stable_pressed_ && !holding_ && now_ms - press_start_ms_ >= hold_ms_) {
    holding_ = true;
    return Event::kHoldStart;
  }
  return Event::kNone;
}
