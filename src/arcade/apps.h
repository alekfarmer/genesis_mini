#pragma once

#include <stdint.h>

#include "arcade_app.h"

/// The games in the arcade menu, in order (defined in apps.cpp).
extern arcade::App* const kGames[];
extern const uint8_t kGameCount;
