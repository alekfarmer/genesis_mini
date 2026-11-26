#pragma once

// Module types this repo knows how to drive, for use in board_config.h.
// The value is the module's Axiometa catalog number (AX22-00NN -> NN), so the
// config reads like the labels on the modules. 0 means an empty port.
//
// To support a new module: add it here, add it to IS_KNOWN_MODULE and give it
// pins in pins.h (from the catalog's get_module entry, never from memory).

#define MOD_NONE 0
#define MOD_ENCODER 3      // AX22-0003 Rotary Encoder with push button
#define MOD_DHT11 11       // AX22-0011 DHT11 Temperature & Humidity Sensor
#define MOD_BUZZER 18      // AX22-0018 Passive Buzzer
#define MOD_TFT 34         // AX22-0034 ST7735 160x80 IPS TFT Display
#define MOD_LED_BUTTON 50  // AX22-0050 Tactile LED Button

#define IS_KNOWN_MODULE(m)                                                                    \
  ((m) == MOD_NONE || (m) == MOD_ENCODER || (m) == MOD_DHT11 || (m) == MOD_BUZZER || \
   (m) == MOD_TFT || (m) == MOD_LED_BUTTON)
