#pragma once

#include <stdint.h>

#include "blockfall_core.h"

namespace blockfall {

/// Draw blockfall into panel::canvas() in portrait (80x160): score, level and
/// the next piece in a 20 px strip on top, the 10x20 well below in 7 px
/// squares, and a "game over" panel with the best score. The caller pushes
/// the canvas to the panel.
void drawGame(const Game& game, uint32_t best);

}  // namespace blockfall
