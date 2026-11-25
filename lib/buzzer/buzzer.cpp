#include "buzzer.h"

#include <Arduino.h>

Buzzer::Buzzer(uint8_t pin) : pin_(pin) {}

void Buzzer::play(uint16_t freq_hz) {
  // Catalog note: always noTone() before a new tone. Both return void in
  // core 3.3.x, so there is nothing to check; a silent buzzer is caught in
  // the manual check (README).
  noTone(pin_);
  if (freq_hz > 0) {
    tone(pin_, freq_hz);
  }
}
