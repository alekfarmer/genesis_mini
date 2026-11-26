#pragma once

// Alternative wiring: the Tactile LED Button in port 3 instead of the DHT11.
// Built by the `arcade_led_button` project in platformio.ini, so the games
// that need the button (Chameleon) are compiled and checked on every
// `make verify`, and so you can flash that wiring without editing
// board_config.h.

#include "modules.h"

#define PORT1_MODULE MOD_TFT
#define PORT2_MODULE MOD_BUZZER
#define PORT3_MODULE MOD_LED_BUTTON
#define PORT4_MODULE MOD_ENCODER
