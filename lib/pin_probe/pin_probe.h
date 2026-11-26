#pragma once

/// Classifies what is connected to a pin from two reads: one with the chip's
/// weak internal pull-up on, one with its weak pull-down on. A pin with
/// nothing attached follows the internal pull; a module with its own, stronger
/// pull resistor (or a driven output) holds the pin at its level either way.
/// Pure logic, host-tested.
namespace pin_probe {

enum class PinState {
  kFloating,    // follows the internal pull: nothing (or only high impedance) attached
  kPulledHigh,  // reads HIGH even against the pull-down: an external pull-up or driver
  kPulledLow,   // reads LOW even against the pull-up: an external pull-down or driver
  kOdd,         // LOW with pull-up but HIGH with pull-down: changing during the reads
};

PinState classify(bool high_with_pullup, bool high_with_pulldown);

/// Short label for serial output, e.g. "floating".
const char* describe(PinState state);

}  // namespace pin_probe
