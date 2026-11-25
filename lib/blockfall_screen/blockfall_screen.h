#pragma once

#include <stdint.h>

#include "blockfall_core.h"

/// Draws blockfall on the kit's screen in portrait (80x160): score, level and
/// the next piece in a 20 px strip on top, the 10x20 well below in 7 px
/// squares, and a "game over" panel with the best score.
class BlockfallScreen {
 public:
  /// @param cs, rst, dc  the screen's port pins (pins::kTftCs/kTftRst/kTftDc).
  BlockfallScreen(uint8_t cs, uint8_t rst, uint8_t dc);

  /// Start the panel in portrait. Returns false on failure (see panel::begin).
  bool begin();

  /// Render the whole game and push it to the panel. Call only when
  /// game.revision() or best changed, at most ~30 fps.
  void draw(const blockfall::Game& game, uint32_t best);

 private:
  uint8_t cs_, rst_, dc_;
  bool ready_ = false;
};
