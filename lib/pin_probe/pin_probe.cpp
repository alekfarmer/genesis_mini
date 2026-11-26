#include "pin_probe.h"

namespace pin_probe {

PinState classify(bool high_with_pullup, bool high_with_pulldown) {
  if (high_with_pullup && !high_with_pulldown) return PinState::kFloating;
  if (high_with_pullup && high_with_pulldown) return PinState::kPulledHigh;
  if (!high_with_pullup && !high_with_pulldown) return PinState::kPulledLow;
  return PinState::kOdd;
}

const char* describe(PinState state) {
  switch (state) {
    case PinState::kFloating:   return "floating";
    case PinState::kPulledHigh: return "pulled HIGH";
    case PinState::kPulledLow:  return "pulled LOW";
    case PinState::kOdd:        return "changing";
  }
  return "?";
}

}  // namespace pin_probe
