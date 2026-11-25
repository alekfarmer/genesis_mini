#include "menu_app.h"

#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"

namespace ui = arcade::ui;

namespace {
// Layout, 160 x 80: title row y 2 (size 1), rule at y 11, then five 13 px
// rows from y 14 (ends y 79). Each row: name at x 8, "[TALL]" tag for
// portrait games right after it (name <= 10 chars -> ends by x 110), best
// score right-aligned at x 156 (<= 6 digits = 36 px, starts at x >= 120).
constexpr int16_t kRowTop = 14;
constexpr int16_t kRowHeight = 13;
constexpr int16_t kNameX = 8;
constexpr int16_t kScoreRight = 156;
}  // namespace

MenuApp::MenuApp(arcade::App* const* apps, uint8_t count)
    : apps_(apps), count_(count > kMaxApps ? kMaxApps : count), model_(count_, kVisibleRows) {}

void MenuApp::enter(uint32_t now_ms) {
  (void)now_ms;
  for (uint8_t i = 0; i < count_; ++i) {
    const char* key = apps_[i]->scoreKey();
    best_[i] = key != nullptr ? arcade::loadBest(key) : 0;
  }
  launch_ = nullptr;
  arcade::setLed(arcade::kLedNormal);
  dirty_ = true;
}

void MenuApp::update(const arcade::Input& in, uint32_t now_ms) {
  (void)now_ms;
  if (in.knob_steps != 0) {
    model_.move(in.knob_steps);
    dirty_ = true;
  }
  if (in.knob_pressed && count_ > 0) {
    launch_ = apps_[model_.selected()];
  }
}

arcade::App* MenuApp::takeLaunch() {
  arcade::App* a = launch_;
  launch_ = nullptr;
  return a;
}

void MenuApp::draw() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());
  ui::text(4, 2, 1, ui::accent(), "ARCADE");
  ui::textRight(158, 2, 1, ui::dim(), "HOLD KNOB: MENU");
  c.drawFastHLine(0, 11, 160, ui::border());

  const uint16_t highlight = conversions::panelColor(40, 60, 110);
  char score[12];
  for (uint8_t row = 0; row < kVisibleRows; ++row) {
    const uint8_t i = static_cast<uint8_t>(model_.firstVisible() + row);
    if (i >= count_) break;
    const int16_t y = static_cast<int16_t>(kRowTop + row * kRowHeight);
    const bool selected = (i == model_.selected());
    if (selected) {
      c.fillRect(0, y, 160, kRowHeight, highlight);
      ui::text(2, y + 3, 1, ui::accent(), ">");
    }
    ui::text(kNameX, y + 3, 1, selected ? ui::white() : ui::dim(), apps_[i]->name());
    if (apps_[i]->orientation() != panel::Orientation::kLandscape) {
      const int16_t tag_x = static_cast<int16_t>(kNameX + ui::textWidth(apps_[i]->name(), 1) + 6);
      ui::text(tag_x, y + 3, 1, ui::accent(), "[TALL]");
    }
    if (apps_[i]->scoreKey() != nullptr && best_[i] > 0) {
      snprintf(score, sizeof(score), "%" PRIu32, best_[i] > 999999u ? 999999u : best_[i]);
      ui::textRight(kScoreRight, y + 3, 1, selected ? ui::white() : ui::dim(), score);
    }
  }
  dirty_ = false;
}
