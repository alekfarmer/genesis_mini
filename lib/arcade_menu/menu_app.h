#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "menu_core.h"

/// The arcade's start screen: a list of games with their best scores. Turn
/// the knob to choose, press it to start. Landscape.
class MenuApp : public arcade::App {
 public:
  static constexpr uint8_t kMaxApps = 12;

  /// @param apps  the games, in menu order; must outlive the menu.
  /// @param count number of games (at most kMaxApps).
  MenuApp(arcade::App* const* apps, uint8_t count);

  const char* name() const override { return "MENU"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override { return dirty_; }
  void draw() override;

  /// The game the player just chose, once; nullptr otherwise.
  arcade::App* takeLaunch();

 private:
  static constexpr uint8_t kVisibleRows = 5;

  arcade::App* const* apps_;
  uint8_t count_;
  MenuModel model_;
  uint32_t best_[kMaxApps] = {};
  arcade::App* launch_ = nullptr;
  bool dirty_ = true;
};
