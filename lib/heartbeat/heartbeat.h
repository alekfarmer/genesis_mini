#pragma once

#include <stdint.h>

/// Heartbeat blink pattern for the status LED: two short pulses, then a pause
/// ("lub-dub"). Pure logic, so it is unit-tested on the host; the LED driver
/// only turns the returned level into a colour.
namespace heartbeat {

/// Length of one full pattern, in milliseconds.
constexpr uint32_t kCycleMs = 1200;
/// Length of each pulse.
constexpr uint32_t kPulseMs = 80;
/// Start of the second pulse, measured from the start of the cycle.
constexpr uint32_t kSecondPulseStartMs = 200;

/// True if the LED should be lit at time now_ms. The pattern repeats every
/// kCycleMs, and is correct across millis() rollover only in the sense that it
/// keeps repeating (a single glitched cycle at wrap is acceptable for an LED).
bool isOn(uint32_t now_ms);

}  // namespace heartbeat
