#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "chameleon_core.h"

/// Chameleon for the arcade. Needs the Tactile LED Button module. Landscape.
///   Knob, turn          steer
///   Knob, press         stop / walk
///   LED button, press   camouflage (next to an object; its LED lights when
///                       camouflage is possible)
///   Any press           new game, from the game-over panel
/// The NeoPixel shows your camouflage colour.
class ChameleonApp : public arcade::App {
 public:
  ChameleonApp();

  const char* name() const override { return "CHAMELEON"; }
  const char* scoreKey() const override { return "chameleon"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override { return ui_dirty_ || game_.revision() != drawn_revision_; }
  void draw() override;
  void exit() override;

 private:
  void newGame(uint32_t now_ms);
  void onCaught(uint32_t now_ms);

  chameleon::Game game_;
  uint32_t best_ = 0;
  bool new_best_ = false;
  uint32_t last_tick_ms_ = 0;
  uint32_t over_ms_ = 0;
  bool was_camouflaged_ = false;
  uint32_t drawn_revision_ = UINT32_MAX;
  bool ui_dirty_ = true;
};
