#pragma once

#include <stdint.h>

/// The kit's passive buzzer (AX22-0018), driven with tone()/noTone(). It only
/// plays what it is told; timing comes from ToneSequence.
class Buzzer {
 public:
  /// @param pin the port's IO1 alias (pins::kBuzzer).
  explicit Buzzer(uint8_t pin);

  /// Play freq_hz until told otherwise; 0 = silence.
  void play(uint16_t freq_hz);

 private:
  uint8_t pin_;
  bool playing_ = false;  // a tone is currently running on pin_
};
