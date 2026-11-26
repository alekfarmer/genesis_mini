#include "hideseek_app.h"

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;
using conversions::panelColor;
using hideseek::Hint;
using hideseek::Kind;
using hideseek::Phase;

namespace {

const ToneSequence::Step kHideSound[] = {{660, 40}, {880, 60}};
const ToneSequence::Step kHotSound[] = {{1760, 60}, {0, 30}, {1760, 60}};
const ToneSequence::Step kWarmSound[] = {{988, 90}};
const ToneSequence::Step kColdSound[] = {{392, 140}};
const ToneSequence::Step kFoundSound[] = {{784, 80}, {988, 80}, {1175, 80}, {1568, 200}};
const ToneSequence::Step kNotFoundSound[] = {{330, 150}, {0, 40}, {262, 300}};

constexpr StatusLed::Color kLedHot = {20, 2, 0};
constexpr StatusLed::Color kLedWarm = {14, 8, 0};
constexpr StatusLed::Color kLedCold = {0, 4, 18};

template <size_t N>
void play(const ToneSequence::Step (&steps)[N], uint32_t now_ms) {
  arcade::playSound(steps, N, now_ms);
}

const char* hintWord(Hint h) {
  switch (h) {
    case Hint::kHot:  return "HOT!";
    case Hint::kWarm: return "WARM";
    case Hint::kCold: return "COLD";
    default:          return "";
  }
}

uint16_t hintColor(Hint h) {
  switch (h) {
    case Hint::kHot:  return panelColor(255, 70, 50);
    case Hint::kWarm: return panelColor(255, 170, 40);
    default:          return panelColor(90, 160, 255);
  }
}

void drawSpot(const hideseek::Spot& s) {
  GFXcanvas16& c = panel::canvas();
  const int16_t x = s.x, y = s.y;
  const uint16_t brown = panelColor(120, 75, 40);
  switch (s.kind) {
    case Kind::kTree:
      c.fillRect(x - 1, y, 3, 6, brown);
      c.fillCircle(x, y - 2, 5, panelColor(40, 150, 60));
      break;
    case Kind::kBush:
      c.fillCircle(x - 3, y + 1, 4, panelColor(60, 170, 70));
      c.fillCircle(x + 3, y + 1, 4, panelColor(60, 170, 70));
      c.fillCircle(x, y - 2, 4, panelColor(80, 190, 90));
      break;
    case Kind::kRock:
      c.fillCircle(x, y, 5, panelColor(130, 130, 140));
      c.drawPixel(x - 2, y - 2, panelColor(200, 200, 210));
      break;
    case Kind::kShed:
      c.fillRect(x - 6, y - 3, 12, 9, brown);
      c.fillTriangle(x - 7, y - 3, x + 7, y - 3, x, y - 9, panelColor(170, 60, 50));
      c.fillRect(x - 1, y + 1, 3, 5, panelColor(60, 35, 20));
      break;
    case Kind::kLog:
      c.fillRoundRect(x - 8, y - 2, 16, 5, 2, brown);
      c.fillCircle(x + 7, y, 2, panelColor(190, 140, 90));
      break;
    case Kind::kWell:
      c.fillCircle(x, y, 5, panelColor(140, 140, 150));
      c.fillCircle(x, y, 3, panelColor(20, 40, 90));
      break;
  }
}

void statusLine(const char* left, const char* right, uint16_t right_color) {
  panel::canvas().fillRect(0, 0, 160, 10, ui::black());
  ui::text(2, 1, 1, ui::white(), left);
  ui::textRight(158, 1, 1, right_color, right);
}

}  // namespace

void HideSeekApp::enter(uint32_t now_ms) {
  (void)now_ms;
  game_.reset();
  arcade::setLed(arcade::kLedNormal);
  drawn_revision_ = UINT32_MAX;
  Serial.println("[hideseek] new match: player 1 hides first");
}

void HideSeekApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (in.knob_steps != 0) {
    game_.moveCursor(in.knob_steps);
  }
  if (in.knob_pressed || in.button_tap) {
    const Phase before = game_.phase();
    const int searches = game_.searchesThisRound();
    game_.press();
    afterPress(before, searches, now_ms);
  }
}

void HideSeekApp::afterPress(Phase before, int searches_before, uint32_t now_ms) {
  const Phase now_phase = game_.phase();
  if (before == Phase::kHide && now_phase == Phase::kPassToSeeker) {
    play(kHideSound, now_ms);
    arcade::setLed(arcade::kLedNormal);
  } else if (before == Phase::kSeek && game_.searchesThisRound() > searches_before) {
    if (now_phase == Phase::kRoundResult) {
      if (game_.foundThisRound()) {
        play(kFoundSound, now_ms);
        arcade::setLed(arcade::kLedFlash);
      } else {
        play(kNotFoundSound, now_ms);
        arcade::setLed(arcade::kLedBad);
      }
      Serial.printf("[hideseek] round %d: player %d %s after %d search(es); totals P1 %" PRIu32 ", P2 %" PRIu32 "\n",
                    game_.round() + 1, game_.seeker() + 1, game_.foundThisRound() ? "found them" : "gave up",
                    game_.searchesThisRound(), game_.total(0), game_.total(1));
    } else {
      switch (game_.lastHint()) {
        case Hint::kHot:  play(kHotSound, now_ms);  arcade::setLed(kLedHot);  break;
        case Hint::kWarm: play(kWarmSound, now_ms); arcade::setLed(kLedWarm); break;
        default:          play(kColdSound, now_ms); arcade::setLed(kLedCold); break;
      }
    }
  } else if (now_phase == Phase::kPassToHider || now_phase == Phase::kHide) {
    arcade::setLed(arcade::kLedNormal);
  } else if (now_phase == Phase::kMatchOver) {
    Serial.printf("[hideseek] match over: P1 %" PRIu32 ", P2 %" PRIu32 "\n", game_.total(0), game_.total(1));
  }
}

void HideSeekApp::drawMap(bool show_cursor, bool show_searched, bool reveal) {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(panelColor(25, 70, 35));
  for (int i = 0; i < hideseek::kSpots; ++i) {
    drawSpot(hideseek::spot(i));
  }
  if (show_searched) {
    for (int i = 0; i < hideseek::kSpots; ++i) {
      if (!game_.searched(i)) continue;
      const hideseek::Spot& s = hideseek::spot(i);
      c.drawLine(s.x - 4, s.y - 4, s.x + 4, s.y + 4, ui::white());
      c.drawLine(s.x - 4, s.y + 4, s.x + 4, s.y - 4, ui::white());
    }
  }
  if (reveal) {
    const hideseek::Spot& s = hideseek::spot(game_.hiddenSpot());
    c.drawCircle(s.x, s.y, 9, panelColor(255, 80, 220));
    c.drawCircle(s.x, s.y, 10, panelColor(255, 80, 220));
  }
  if (show_cursor) {
    const hideseek::Spot& s = hideseek::spot(game_.cursor());
    c.drawCircle(s.x, s.y, 9, ui::accent());
    c.drawCircle(s.x, s.y, 10, ui::accent());
  }
}

void HideSeekApp::drawPassScreen(int player_up, int player_away, const char* action) {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());
  char line[48];  // room for any int; the text itself is kept within 26 chars
  snprintf(line, sizeof(line), "PLAYER %d", player_up + 1);
  ui::textCentered(8, 2, ui::accent(), line);
  ui::textCentered(30, 1, ui::white(), "TAKE THE BOARD");
  snprintf(line, sizeof(line), "PLAYER %d: LOOK AWAY!", player_away + 1);
  ui::textCentered(44, 1, ui::bad(), line);
  ui::textCentered(64, 1, ui::dim(), action);
}

void HideSeekApp::drawMatchOver() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());
  ui::textCentered(4, 2, ui::accent(), "MATCH OVER");
  char line[48];  // room for any int; the text itself is kept within 26 chars
  snprintf(line, sizeof(line), "P1: %" PRIu32 "   P2: %" PRIu32, game_.total(0), game_.total(1));
  ui::textCentered(28, 1, ui::white(), line);
  ui::textCentered(38, 1, ui::dim(), "FEWEST SEARCHES WINS");  // 20 chars = 120 px
  const int w = game_.winner();
  if (w < 0) {
    ui::textCentered(52, 1, ui::good(), "IT'S A TIE!");
  } else {
    snprintf(line, sizeof(line), "PLAYER %d WINS!", w + 1);
    ui::textCentered(52, 1, ui::good(), line);
  }
  ui::textCentered(68, 1, ui::dim(), "PRESS TO PLAY AGAIN");
}

void HideSeekApp::draw() {
  char left[48];
  char right[32];
  switch (game_.phase()) {
    case Phase::kHide:
      drawMap(true, false, false);
      snprintf(left, sizeof(left), "P%d HIDES: PICK A SPOT", game_.hider() + 1);
      statusLine(left, "", ui::white());
      break;
    case Phase::kPassToSeeker:
      drawPassScreen(game_.seeker(), game_.hider(), "PRESS TO START SEEKING");
      break;
    case Phase::kSeek:
      drawMap(true, true, false);
      snprintf(left, sizeof(left), "P%d SEEKS %d/%d", game_.seeker() + 1, game_.searchesThisRound(),
               hideseek::kMaxSearches);
      statusLine(left, hintWord(game_.lastHint()), hintColor(game_.lastHint()));
      break;
    case Phase::kRoundResult:
      drawMap(false, true, true);
      if (game_.foundThisRound()) {
        snprintf(left, sizeof(left), "FOUND IN %d!", game_.searchesThisRound());
      } else {
        snprintf(left, sizeof(left), "NOT FOUND: +%d", hideseek::kMissPenalty);
      }
      snprintf(right, sizeof(right), "%" PRIu32 "-%" PRIu32, game_.total(0), game_.total(1));
      statusLine(left, right, ui::dim());
      break;
    case Phase::kPassToHider:
      drawPassScreen(game_.hider(), game_.seeker(), "PRESS TO HIDE");
      break;
    case Phase::kMatchOver:
      drawMatchOver();
      break;
  }
  drawn_revision_ = game_.revision();
}
