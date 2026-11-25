#pragma once

// The one place that says which pin does what in this project.
//
// Source of truth for the pin numbers: the Axiometa Genesis Mini variant
// (variants/axiometa_genesis_mini/pins_arduino.h), which defines the port
// aliases P{N}_IO0..2 and the board pins. Which module sits in which port:
// docs/modules.md. Per-module pin roles: the Axiometa catalog (get_module).
// All pins are 3.3 V logic.

#include <Arduino.h>

namespace pins {

// ---- On-board -------------------------------------------------------------
constexpr uint8_t kNeoPixel = RGB_LED;        // GPIO21, WS2812
constexpr uint8_t kUserButton = USER_BUTTON;  // GPIO45
constexpr uint8_t kBatterySense = BAT_SENSE;  // GPIO8, ADC via /2 divider
// I2C (shared by all ports and STEMMA QT): SDA = GPIO10, SCL = GPIO11, set by
// the variant; use Wire.begin() with no arguments.
// SPI (shared by all ports): MOSI = 12, MISO = 13, SCK = 14, from the variant.

// ---- Port 1: ST7735 160x80 TFT (AX22-0034), SPI ---------------------------
constexpr uint8_t kTftCs = P1_IO0;   // chip select
constexpr uint8_t kTftRst = P1_IO1;  // reset (GPIO3: strapping pin, fine after boot)
constexpr uint8_t kTftDc = P1_IO2;   // data/command

// ---- Port 2: Passive buzzer (AX22-0018) -----------------------------------
constexpr uint8_t kBuzzer = P2_IO1;  // PWM tone output

// ---- Port 3: DHT11 temperature & humidity sensor (AX22-0011) --------------
constexpr uint8_t kDht = P3_IO1;  // single-wire data, pull-up on the module

// ---- Port 4: Rotary encoder with push button (AX22-0003) ------------------
constexpr uint8_t kEncoderButton = P4_IO0;  // active HIGH, module has pull-down
constexpr uint8_t kEncoderClk = P4_IO1;
constexpr uint8_t kEncoderDt = P4_IO2;

}  // namespace pins
