#pragma once

#include <stdint.h>

/// Non-blocking fixed-interval timer, used instead of delay() in loop().
///
/// Feed it the current time (e.g. millis()); due() returns true once per
/// interval. Uses unsigned subtraction, so it stays correct when the 32-bit
/// millisecond counter wraps (every ~49.7 days). Pure logic, no Arduino
/// dependency, so it is unit-tested on the host.
class Periodic {
 public:
  /// @param interval_ms time between firings; 0 means "due on every call".
  /// @param start_ms    time the first interval is measured from.
  explicit Periodic(uint32_t interval_ms, uint32_t start_ms = 0);

  /// True if at least one interval has elapsed since the last firing. When it
  /// fires, the next deadline advances by whole intervals, so a late call does
  /// not cause a burst of catch-up firings and the cadence does not drift.
  bool due(uint32_t now_ms);

  /// Restart the interval from now_ms.
  void reset(uint32_t now_ms);

 private:
  uint32_t interval_ms_;
  uint32_t last_ms_;
};
