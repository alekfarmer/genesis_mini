#pragma once

// ======================================================================
//  Which module is plugged into which AX22 port. EDIT THIS to match
//  your board, then rebuild. This file is the source of truth for wiring.
// ======================================================================
//
// Use the names from include/modules.h (MOD_NONE for an empty port). Each
// module type can be in one port at most.
//
// Projects use this to decide what to build:
//   - The arcade only lists games whose modules are configured (see
//     src/arcade/apps.cpp), and needs at least the screen and the encoder.
//   - kit_demo needs the screen, buzzer, DHT11 and encoder. Without them it
//     builds a stub that explains what's missing.
//
// This is the Genesis Mini Starter Kit as plugged in on this board.

#include "modules.h"

#define PORT1_MODULE MOD_TFT
#define PORT2_MODULE MOD_BUZZER
#define PORT3_MODULE MOD_DHT11
#define PORT4_MODULE MOD_ENCODER
