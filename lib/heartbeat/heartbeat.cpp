#include "heartbeat.h"

namespace heartbeat {

bool isOn(uint32_t now_ms) {
  const uint32_t t = now_ms % kCycleMs;
  const bool first_pulse = t < kPulseMs;
  const bool second_pulse = t >= kSecondPulseStartMs && t < kSecondPulseStartMs + kPulseMs;
  return first_pulse || second_pulse;
}

}  // namespace heartbeat
