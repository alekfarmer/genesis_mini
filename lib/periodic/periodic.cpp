#include "periodic.h"

Periodic::Periodic(uint32_t interval_ms, uint32_t start_ms)
    : interval_ms_(interval_ms), last_ms_(start_ms) {}

bool Periodic::due(uint32_t now_ms) {
  if (interval_ms_ == 0) {
    return true;
  }
  // Unsigned subtraction gives the right elapsed time even across a wrap.
  const uint32_t elapsed = now_ms - last_ms_;
  if (elapsed < interval_ms_) {
    return false;
  }
  // Advance by whole intervals: one firing for a late call, no drift.
  last_ms_ += (elapsed / interval_ms_) * interval_ms_;
  return true;
}

void Periodic::reset(uint32_t now_ms) { last_ms_ = now_ms; }
