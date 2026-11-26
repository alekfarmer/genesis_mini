#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "hideseek_core.h"

/// Hide & Seek for the arcade: two players, one board, passed between them.
/// Landscape.
///   Knob, turn                      move the cursor between hiding spots
///   Knob press or button tap        hide here / search here / continue
class HideSeekApp : public arcade::App {
 public:
  const char* name() const override { return "HIDE+SEEK"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override { return game_.revision() != drawn_revision_; }
  void draw() override;

 private:
  void afterPress(hideseek::Phase before, int searches_before, uint32_t now_ms);
  void drawMap(bool show_cursor, bool show_searched, bool reveal);
  void drawPassScreen(int player_up, int player_away, const char* action);
  void drawMatchOver();

  hideseek::Game game_;
  uint32_t drawn_revision_ = UINT32_MAX;
};
