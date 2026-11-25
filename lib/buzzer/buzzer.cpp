#include "buzzer.h"

#include <Arduino.h>

Buzzer::Buzzer(uint8_t pin) : pin_(pin) {}

void Buzzer::play(uint16_t freq_hz) {
  // Catalog note: noTone() before a new tone. Only when one is running,
  // though: core 3.3.x logs an error for noTone() on an idle pin. Both calls
  // return void, so a silent buzzer is caught in the manual check (README).
  if (playing_) {
    noTone(pin_);
    playing_ = false;
  }
  if (freq_hz > 0) {
    tone(pin_, freq_hz);
    playing_ = true;
  }
}
