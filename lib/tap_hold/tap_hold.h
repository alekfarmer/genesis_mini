#pragma once

#include <stdint.h>

/// Tells a short tap from a long hold on one button, with debouncing.
///
/// A tap is reported when the button is released before hold_ms. A hold
/// starts once the button has been down for hold_ms, and ends on release
/// (no tap then). A level change only counts after it has been stable for
/// settle_ms, which filters contact bounce. Pure logic, host-tested.
class TapHold {
 public:
  enum class Event : uint8_t { kNone, kTap, kHoldStart, kHoldEnd };

  /// @param active_high true if pressed reads HIGH; false for active-low
  ///                    buttons such as the Genesis Mini's on-board button.
  TapHold(bool active_high, uint32_t hold_ms, uint32_t settle_ms);

  /// Feed the raw level every loop(); returns at most one event per call.
  Event update(bool raw_level, uint32_t now_ms);

  /// True while a hold is in progress.
  bool held() const { return holding_; }

 private:
  bool active_high_;
  uint32_t hold_ms_;
  uint32_t settle_ms_;
  bool stable_pressed_ = false;
  bool candidate_pressed_ = false;
  uint32_t candidate_since_ms_ = 0;
  uint32_t press_start_ms_ = 0;
  bool holding_ = false;
};
