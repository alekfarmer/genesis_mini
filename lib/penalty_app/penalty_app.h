#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "penalty_core.h"

/// Penalty Kick for the arcade: you shoot, the computer keeps goal.
/// Landscape.
///   Knob, turn                   aim left / right
///   Knob press or button tap     lock the aim, then stop the power meter to
///                                shoot; continue past result screens
class PenaltyApp : public arcade::App {
 public:
  PenaltyApp();

  const char* name() const override { return "PENALTY"; }
  const char* scoreKey() const override { return "penalty"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override { return ui_dirty_ || game_.revision() != drawn_revision_; }
  void draw() override;

 private:
  void newGame(uint32_t now_ms);
  void onPhaseChange(penalty::Phase from, penalty::Phase to, uint32_t now_ms);

  penalty::Game game_;
  uint32_t best_ = 0;
  bool new_best_ = false;
  uint32_t last_tick_ms_ = 0;
  uint32_t over_ms_ = 0;
  uint32_t drawn_revision_ = UINT32_MAX;
  bool ui_dirty_ = true;
};
