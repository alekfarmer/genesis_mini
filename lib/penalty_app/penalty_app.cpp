#include "penalty_app.h"

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;
using conversions::panelColor;
using penalty::Phase;
using penalty::Result;

namespace {

constexpr int kMaxCatchUpTicks = 5;

const ToneSequence::Step kKickSound[] = {{140, 25}, {90, 25}};
const ToneSequence::Step kGoalSound[] = {{523, 90}, {659, 90}, {784, 90}, {1047, 260}};
const ToneSequence::Step kSavedSound[] = {{300, 60}, {200, 160}};
const ToneSequence::Step kMissSound[] = {{440, 80}, {0, 30}, {330, 200}};
const ToneSequence::Step kWhistleSound[] = {{2600, 120}, {0, 60}, {2600, 300}};

template <size_t N>
void play(const ToneSequence::Step (&steps)[N], uint32_t now_ms) {
  arcade::playSound(steps, N, now_ms);
}

// Layout, 160 x 80: HUD strip y 0-9; goal from the crossbar (y 10-11) down
// to the ground line (y 50), posts at x 28-29 and 130-131; the ball starts on
// the spot at (80, 72); the power meter is at x 148-155, y 54-77.
constexpr int16_t kMeterX = 148;
constexpr int16_t kMeterTop = 54;
constexpr int16_t kMeterHeight = 24;
constexpr int16_t kOverBarPower = 78;  // ~where the ball starts to clear the bar

const char* resultWord(Result r) {
  switch (r) {
    case Result::kGoal:  return "GOAL!";
    case Result::kSaved: return "SAVED";
    case Result::kWide:  return "WIDE";
    case Result::kOver:  return "OVER";
    default:             return "";
  }
}

}  // namespace

PenaltyApp::PenaltyApp() : game_(1) {}

void PenaltyApp::enter(uint32_t now_ms) {
  best_ = arcade::loadBest(scoreKey());
  newGame(now_ms);
}

void PenaltyApp::newGame(uint32_t now_ms) {
  game_.reset(arcade::newSeed());
  new_best_ = false;
  last_tick_ms_ = now_ms;
  arcade::setLed(arcade::kLedNormal);
  ui_dirty_ = true;
  Serial.printf("[penalty] new game (best %" PRIu32 ")\n", best_);
}

void PenaltyApp::onPhaseChange(Phase from, Phase to, uint32_t now_ms) {
  if (to == Phase::kFlight) {
    play(kKickSound, now_ms);
  } else if (from == Phase::kFlight && to == Phase::kResult) {
    switch (game_.lastResult()) {
      case Result::kGoal:
        play(kGoalSound, now_ms);
        arcade::setLed(arcade::kLedFlash);
        break;
      case Result::kSaved:
        play(kSavedSound, now_ms);
        arcade::setLed(arcade::kLedBad);
        break;
      default:
        play(kMissSound, now_ms);
        arcade::setLed(arcade::kLedBad);
        break;
    }
    Serial.printf("[penalty] round %d shot %d: %s (goals %" PRIu32 ")\n", game_.round(), game_.shot(),
                  resultWord(game_.lastResult()), game_.score());
  } else if (to == Phase::kAim) {
    arcade::setLed(arcade::kLedNormal);
  } else if (to == Phase::kRoundOver) {
    play(kWhistleSound, now_ms);
  } else if (to == Phase::kGameOver) {
    over_ms_ = now_ms;
    arcade::setLed(arcade::kLedBad);
    new_best_ = game_.score() > best_;
    if (new_best_) {
      best_ = game_.score();
      arcade::saveBest(scoreKey(), best_);
    }
    ui_dirty_ = true;
    Serial.printf("[penalty] game over: %" PRIu32 " goals in %d round(s)%s\n", game_.score(), game_.round(),
                  new_best_ ? " (new best)" : "");
  }
}

void PenaltyApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (game_.phase() == Phase::kGameOver) {
    if ((in.knob_pressed || in.button_tap) && now_ms - over_ms_ >= arcade::kRestartGuardMs) {
      newGame(now_ms);
    }
    return;
  }
  game_.moveAim(in.knob_steps);
  if (in.knob_pressed || in.button_tap) {
    const Phase before = game_.phase();
    game_.press();
    if (game_.phase() != before) onPhaseChange(before, game_.phase(), now_ms);
  }

  int ticks = 0;
  while (now_ms - last_tick_ms_ >= penalty::kTickMs) {
    if (++ticks > kMaxCatchUpTicks) {
      last_tick_ms_ = now_ms;
      break;
    }
    last_tick_ms_ += penalty::kTickMs;
    const Phase before = game_.phase();
    game_.tick();
    if (game_.phase() != before) onPhaseChange(before, game_.phase(), now_ms);
  }
}

void PenaltyApp::draw() {
  GFXcanvas16& c = panel::canvas();
  const Phase phase = game_.phase();

  // Pitch: night sky above the goal line, grass below.
  c.fillRect(0, 0, 160, penalty::kGroundY, panelColor(15, 20, 45));
  c.fillRect(0, penalty::kGroundY, 160, 80 - penalty::kGroundY, panelColor(30, 120, 50));
  c.drawFastHLine(0, penalty::kGroundY, 160, panelColor(200, 230, 200));
  c.fillCircle(penalty::kBallStartX, penalty::kBallStartY + 3, 1, panelColor(200, 230, 200));  // the spot

  // Net, then the frame on top.
  const uint16_t net = panelColor(70, 75, 100);
  for (int16_t x = penalty::kGoalLeft + 8; x < penalty::kGoalRight; x += 8) {
    c.drawFastVLine(x, penalty::kCrossbarY, penalty::kGroundY - penalty::kCrossbarY, net);
  }
  for (int16_t y = penalty::kCrossbarY + 8; y < penalty::kGroundY; y += 8) {
    c.drawFastHLine(penalty::kGoalLeft, y, penalty::kGoalRight - penalty::kGoalLeft, net);
  }
  c.fillRect(penalty::kGoalLeft - 2, penalty::kCrossbarY - 2, penalty::kGoalRight - penalty::kGoalLeft + 4, 2,
             ui::white());
  c.fillRect(penalty::kGoalLeft - 2, penalty::kCrossbarY, 2, penalty::kGroundY - penalty::kCrossbarY, ui::white());
  c.fillRect(penalty::kGoalRight, penalty::kCrossbarY, 2, penalty::kGroundY - penalty::kCrossbarY, ui::white());

  // Goalie: body, head and arms out.
  const int16_t gx = game_.goalieX();
  const uint16_t kit = panelColor(255, 150, 40);
  c.fillRect(gx - 4, 32, 9, 18, kit);
  c.fillCircle(gx, 28, 3, panelColor(230, 190, 150));
  c.drawFastHLine(gx - 9, 34, 19, kit);

  // Aim marker while aiming or choosing power.
  if (phase == Phase::kAim || phase == Phase::kPower) {
    const int16_t ax = game_.aimX();
    const uint16_t aim = phase == Phase::kAim ? ui::accent() : panelColor(255, 255, 150);
    c.drawFastVLine(ax, 36, 9, aim);
    c.drawFastHLine(ax - 4, 40, 9, aim);
  }

  // Ball, shrinking as it flies away.
  const uint8_t radius = static_cast<uint8_t>(4 - (2 * game_.flightTicks()) / penalty::kFlightTicks);
  if (phase != Phase::kRoundOver && phase != Phase::kGameOver) {
    c.fillCircle(game_.ballX(), game_.ballY(), phase == Phase::kAim || phase == Phase::kPower ? 4 : radius,
                 ui::white());
  }

  // Power meter.
  if (phase == Phase::kPower) {
    c.drawRect(kMeterX - 1, kMeterTop - 1, 10, kMeterHeight + 2, ui::white());
    const int16_t h = static_cast<int16_t>(game_.power() * kMeterHeight / penalty::kMaxPower);
    const uint16_t fill = game_.power() > kOverBarPower ? ui::bad()
                          : game_.power() > penalty::kWildPower ? ui::accent()
                                                                : ui::good();
    c.fillRect(kMeterX, static_cast<int16_t>(kMeterTop + kMeterHeight - h), 8, h, fill);
    const int16_t bar = static_cast<int16_t>(kMeterTop + kMeterHeight - kOverBarPower * kMeterHeight / 100);
    c.drawFastHLine(kMeterX - 2, bar, 12, ui::bad());
  }

  // HUD: round, the five shots of this round, total goals.
  c.fillRect(0, 0, 160, 8, ui::black());
  char line[20];
  snprintf(line, sizeof(line), "ROUND %d", game_.round());
  ui::text(2, 0, 1, ui::white(), line);
  for (int i = 0; i < penalty::kShotsPerRound; ++i) {
    const int16_t x = static_cast<int16_t>(66 + i * 8);
    const Result r = game_.shotResult(i);
    if (r == Result::kNone) {
      c.drawCircle(x, 3, 2, ui::dim());
    } else {
      c.fillCircle(x, 3, 2, r == Result::kGoal ? ui::good() : ui::bad());
    }
  }
  snprintf(line, sizeof(line), "GOALS %" PRIu32, game_.score());
  ui::textRight(158, 0, 1, ui::white(), line);

  if (phase == Phase::kResult) {
    const Result r = game_.lastResult();
    ui::textCentered(56, 2, r == Result::kGoal ? ui::good() : ui::bad(), resultWord(r));
  } else if (phase == Phase::kAim) {
    ui::textCentered(58, 1, ui::dim(), "AIM, THEN PRESS");
  } else if (phase == Phase::kPower) {
    ui::textCentered(58, 1, ui::dim(), "PRESS TO SHOOT");
  } else if (phase == Phase::kRoundOver) {
    c.fillRect(25, 22, 110, 40, ui::panelBg());
    c.drawRect(25, 22, 110, 40, ui::border());
    snprintf(line, sizeof(line), "ROUND %d: %d/%d", game_.round(), game_.goalsThisRound(), penalty::kShotsPerRound);
    ui::textCentered(27, 1, ui::white(), line);
    const bool through = game_.goalsThisRound() >= penalty::kGoalsToAdvance;
    ui::textCentered(38, 1, through ? ui::good() : ui::bad(), through ? "THROUGH!" : "NOT ENOUGH GOALS");
    ui::textCentered(50, 1, ui::dim(), "PRESS KNOB");
  } else if (phase == Phase::kGameOver) {
    ui::gameOverPanel(game_.score(), best_, new_best_);
  }

  drawn_revision_ = game_.revision();
  ui_dirty_ = false;
}
