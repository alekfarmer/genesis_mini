#pragma once

// The one place that says which pin does what in this project.
//
// - Pin numbers: the Axiometa Genesis Mini variant
//   (variants/axiometa_genesis_mini/pins_arduino.h) defines the port aliases
//   P{N}_IO0..2 and the board pins.
// - Which module is in which port: include/board_config.h (or the file named
//   by BOARD_CONFIG_FILE). Each module's pins below are derived from that.
// - Per-module pin roles: the Axiometa catalog (get_module).
// All pins are 3.3 V logic.

#include <Arduino.h>

#ifdef BOARD_CONFIG_FILE
#include BOARD_CONFIG_FILE
#else
#include "board_config.h"
#endif

// ---- Checks on the config ---------------------------------------------------

#if !defined(PORT1_MODULE) || !defined(PORT2_MODULE) || !defined(PORT3_MODULE) || !defined(PORT4_MODULE)
#error "board_config.h must define PORT1_MODULE .. PORT4_MODULE (use MOD_NONE for an empty port)"
#endif
#if !IS_KNOWN_MODULE(PORT1_MODULE) || !IS_KNOWN_MODULE(PORT2_MODULE) || !IS_KNOWN_MODULE(PORT3_MODULE) || \
    !IS_KNOWN_MODULE(PORT4_MODULE)
#error "board_config.h names a module this repo doesn't know; see include/modules.h"
#endif

#define MODULE_SAME_(a, b) ((a) != MOD_NONE && (a) == (b))
#if MODULE_SAME_(PORT1_MODULE, PORT2_MODULE) || MODULE_SAME_(PORT1_MODULE, PORT3_MODULE) || \
    MODULE_SAME_(PORT1_MODULE, PORT4_MODULE) || MODULE_SAME_(PORT2_MODULE, PORT3_MODULE) || \
    MODULE_SAME_(PORT2_MODULE, PORT4_MODULE) || MODULE_SAME_(PORT3_MODULE, PORT4_MODULE)
#error "board_config.h puts the same module type in two ports; each type is supported once"
#endif

// ---- Which modules are present (usable in #if) ------------------------------

/// Port 1-4 holding module type m, or 0 if it isn't configured.
#define MODULE_PORT(m)              \
  (PORT1_MODULE == (m)   ? 1        \
   : PORT2_MODULE == (m) ? 2        \
   : PORT3_MODULE == (m) ? 3        \
   : PORT4_MODULE == (m) ? 4        \
                         : 0)
#define HAS_MODULE(m) (MODULE_PORT(m) != 0)

#define HAS_TFT HAS_MODULE(MOD_TFT)
#define HAS_BUZZER HAS_MODULE(MOD_BUZZER)
#define HAS_DHT11 HAS_MODULE(MOD_DHT11)
#define HAS_ENCODER HAS_MODULE(MOD_ENCODER)
#define HAS_LED_BUTTON HAS_MODULE(MOD_LED_BUTTON)

namespace pins {

/// The GPIO for IO0/IO1/IO2 (`io` = 0..2) of AX22 port 1..4.
constexpr uint8_t portPin(int port, int io) {
  return port == 1   ? (io == 0 ? P1_IO0 : io == 1 ? P1_IO1 : P1_IO2)
         : port == 2 ? (io == 0 ? P2_IO0 : io == 1 ? P2_IO1 : P2_IO2)
         : port == 3 ? (io == 0 ? P3_IO0 : io == 1 ? P3_IO1 : P3_IO2)
                     : (io == 0 ? P4_IO0 : io == 1 ? P4_IO1 : P4_IO2);
}

/// Module type configured in port 1..4 (a MOD_* value).
constexpr int portModule(int port) {
  return port == 1 ? PORT1_MODULE : port == 2 ? PORT2_MODULE : port == 3 ? PORT3_MODULE : PORT4_MODULE;
}

// ---- On-board ---------------------------------------------------------------
constexpr uint8_t kNeoPixel = RGB_LED;        // GPIO21, WS2812
constexpr uint8_t kUserButton = USER_BUTTON;  // GPIO45, active-low (catalog)
constexpr uint8_t kBatterySense = BAT_SENSE;  // GPIO8, ADC via /2 divider
// I2C (shared by all ports and STEMMA QT): SDA = GPIO10, SCL = GPIO11, set by
// the variant; use Wire.begin() with no arguments.
// SPI (shared by all ports): MOSI = 12, MISO = 13, SCK = 14, from the variant.

// ---- Port modules (only defined when configured) ----------------------------

#if HAS_TFT  // ST7735 160x80 TFT (AX22-0034), SPI
constexpr uint8_t kTftCs = portPin(MODULE_PORT(MOD_TFT), 0);   // chip select
constexpr uint8_t kTftRst = portPin(MODULE_PORT(MOD_TFT), 1);  // reset (P1_IO1 = GPIO3 is a strapping pin, fine after boot)
constexpr uint8_t kTftDc = portPin(MODULE_PORT(MOD_TFT), 2);   // data/command
#endif

#if HAS_BUZZER  // Passive buzzer (AX22-0018)
constexpr uint8_t kBuzzer = portPin(MODULE_PORT(MOD_BUZZER), 1);  // PWM tone output
#endif

#if HAS_DHT11  // DHT11 temperature & humidity (AX22-0011)
constexpr uint8_t kDht = portPin(MODULE_PORT(MOD_DHT11), 1);  // single-wire data, pull-up on the module
#endif

#if HAS_ENCODER  // Rotary encoder with push button (AX22-0003)
constexpr uint8_t kEncoderButton = portPin(MODULE_PORT(MOD_ENCODER), 0);  // active HIGH, module has pull-down
constexpr uint8_t kEncoderClk = portPin(MODULE_PORT(MOD_ENCODER), 1);
constexpr uint8_t kEncoderDt = portPin(MODULE_PORT(MOD_ENCODER), 2);
#endif

#if HAS_LED_BUTTON  // Tactile LED Button (AX22-0050)
constexpr uint8_t kLedButton = portPin(MODULE_PORT(MOD_LED_BUTTON), 1);     // LOW when pressed; pull-up + RC debounce on module
constexpr uint8_t kLedButtonLed = portPin(MODULE_PORT(MOD_LED_BUTTON), 2);  // HIGH = LED on
#endif

}  // namespace pins
